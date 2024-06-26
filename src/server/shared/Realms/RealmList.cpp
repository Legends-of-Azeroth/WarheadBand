/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RealmList.h"
#include "DatabaseEnv.h"
#include "DeadlineTimer.h"
#include "IoContext.h"
#include "IoContextMgr.h"
#include "Log.h"
#include "Resolver.h"
#include "Util.h"
#include <boost/asio/ip/tcp.hpp>

RealmList* RealmList::Instance()
{
    static RealmList instance;
    return &instance;
}

// Load the realm list from the database
void RealmList::Initialize(uint32 updateInterval)
{
    _updateInterval = updateInterval;
    _updateTimer = std::make_unique<Warhead::Asio::DeadlineTimer>(sIoContextMgr->GetIoContext());
    _resolver = std::make_unique<Warhead::Asio::Resolver>(sIoContextMgr->GetIoContext());

    LoadBuildInfo();

    // Get the content of the realmlist table in the database
    UpdateRealms(boost::system::error_code());
}

void RealmList::Close()
{
    _updateTimer->cancel();
}

void RealmList::LoadBuildInfo()
{
    //                                                              0             1              2              3      4                5                6
    if (auto result = AuthDatabase.Query("SELECT majorVersion, minorVersion, bugfixVersion, hotfixVersion, build, winChecksumSeed, macChecksumSeed FROM build_info ORDER BY build ASC"))
    {
        for (auto& row : *result)
        {
            auto [majorVersion, minorVersion, bugfixVersion, hotfixVersion, build, windowsHash, macHash] =
                row.FetchTuple<uint32, uint32, uint32, std::string, uint32, std::string, std::string>();

            RealmBuildInfo& realmBuildInfo = _builds.emplace_back();
            realmBuildInfo.MajorVersion = majorVersion;
            realmBuildInfo.MinorVersion = minorVersion;
            realmBuildInfo.BugfixVersion = bugfixVersion;
            realmBuildInfo.Build = build;

            if (hotfixVersion.length() < realmBuildInfo.HotfixVersion.size())
                std::copy(hotfixVersion.begin(), hotfixVersion.end(), realmBuildInfo.HotfixVersion.begin());
            else
                std::fill(hotfixVersion.begin(), hotfixVersion.end(), '\0');

            if (windowsHash.length() == realmBuildInfo.WindowsHash.size() * 2)
                HexStrToByteArray(windowsHash, realmBuildInfo.WindowsHash);

            if (macHash.length() == realmBuildInfo.MacHash.size() * 2)
                HexStrToByteArray(macHash, realmBuildInfo.MacHash);
        }
    }
}

void RealmList::UpdateRealm(RealmHandle const& id, uint32 build, std::string const& name,
    boost::asio::ip::address&& address, boost::asio::ip::address&& localAddr, boost::asio::ip::address&& localSubmask,
    uint16 port, uint8 icon, RealmFlags flag, uint8 timezone, AccountTypes allowedSecurityLevel, float population)
{
    // Create new if not exist or update existed
    Realm& realm = _realms[id];

    realm.Id = id;
    realm.Build = build;
    realm.Name = name;
    realm.Type = icon;
    realm.Flags = flag;
    realm.Timezone = timezone;
    realm.AllowedSecurityLevel = allowedSecurityLevel;
    realm.PopulationLevel = population;

    if (!realm.ExternalAddress || *realm.ExternalAddress != address)
        realm.ExternalAddress = std::make_unique<boost::asio::ip::address>(std::move(address));

    if (!realm.LocalAddress || *realm.LocalAddress != localAddr)
        realm.LocalAddress = std::make_unique<boost::asio::ip::address>(std::move(localAddr));

    if (!realm.LocalSubnetMask || *realm.LocalSubnetMask != localSubmask)
        realm.LocalSubnetMask = std::make_unique<boost::asio::ip::address>(std::move(localSubmask));

    realm.Port = port;
}

void RealmList::UpdateRealms(boost::system::error_code const& error)
{
    if (error)
        return;

    LOG_DEBUG("server.authserver", "Updating Realm List...");

    auto stmt = AuthDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST);
    auto result = AuthDatabase.Query(stmt);

    std::map<RealmHandle, std::string> existingRealms;
    for (auto const& p : _realms)
        existingRealms[p.first] = p.second.Name;

    _realms.clear();

    // Circle through results and add them to the realm map
    if (result)
    {
        for (auto& row : *result)
        {
            try
            {
                uint32 realmId = row[0].Get<uint32>();
                std::string name = row[1].Get<std::string>();
                std::string externalAddressString = row[2].Get<std::string>();
                std::string localAddressString = row[3].Get<std::string>();
                std::string localSubmaskString = row[4].Get<std::string>();

                Optional<boost::asio::ip::tcp::endpoint> externalAddress = _resolver->Resolve(boost::asio::ip::tcp::v4(), externalAddressString, "");
                if (!externalAddress)
                {
                    LOG_ERROR("server.authserver", "Could not resolve address {} for realm \"{}\" id {}", externalAddressString, name, realmId);
                    continue;
                }

                Optional<boost::asio::ip::tcp::endpoint> localAddress = _resolver->Resolve(boost::asio::ip::tcp::v4(), localAddressString, "");
                if (!localAddress)
                {
                    LOG_ERROR("server.authserver", "Could not resolve localAddress {} for realm \"{}\" id {}", localAddressString, name, realmId);
                    continue;
                }

                Optional<boost::asio::ip::tcp::endpoint> localSubmask = _resolver->Resolve(boost::asio::ip::tcp::v4(), localSubmaskString, "");
                if (!localSubmask)
                {
                    LOG_ERROR("server.authserver", "Could not resolve localSubnetMask {} for realm \"{}\" id {}", localSubmaskString, name, realmId);
                    continue;
                }

                uint16 port = row[5].Get<uint16>();
                uint8 icon = row[6].Get<uint8>();

                if (icon == REALM_TYPE_FFA_PVP)
                    icon = REALM_TYPE_PVP;

                if (icon >= MAX_CLIENT_REALM_TYPE)
                    icon = REALM_TYPE_NORMAL;

                RealmFlags flag = RealmFlags(row[7].Get<uint8>());
                uint8 timezone = row[8].Get<uint8>();
                uint8 allowedSecurityLevel = row[9].Get<uint8>();
                float pop = row[10].Get<float>();
                uint32 build = row[11].Get<uint32>();

                RealmHandle id{ realmId };

                UpdateRealm(id, build, name, externalAddress->address(), localAddress->address(), localSubmask->address(), port, icon, flag,
                            timezone, (allowedSecurityLevel <= SEC_ADMINISTRATOR ? AccountTypes(allowedSecurityLevel) : SEC_ADMINISTRATOR), pop);

                if (!existingRealms.count(id))
                    LOG_INFO("server.authserver", "Added realm \"{}\" at {}:{}.", name, externalAddressString, port);
                else
                    LOG_DEBUG("server.authserver", "Updating realm \"{}\" at {}:{}.", name, externalAddressString, port);

                existingRealms.erase(id);
            }
            catch (std::exception const& ex)
            {
                LOG_ERROR("server.authserver", "Realmlist::UpdateRealms has thrown an exception: {}", ex.what());
                ABORT();
            }
        }
    }

    for (auto itr = existingRealms.begin(); itr != existingRealms.end(); ++itr)
        LOG_INFO("server.authserver", "Removed realm \"{}\".", itr->second);

    if (_updateInterval)
    {
        _updateTimer->expires_from_now(boost::posix_time::seconds(_updateInterval));
        _updateTimer->async_wait([this](const boost::system::error_code& errorCode) { UpdateRealms(errorCode); });
    }
}

Realm const* RealmList::GetRealm(RealmHandle const& id) const
{
    auto itr = _realms.find(id);
    if (itr != _realms.end())
        return &itr->second;

    return nullptr;
}

RealmBuildInfo const* RealmList::GetBuildInfo(uint32 build) const
{
    for (RealmBuildInfo const& clientBuild : _builds)
        if (clientBuild.Build == build)
            return &clientBuild;

    return nullptr;
}
