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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "AccountMgr.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SRP6.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "WorldSession.h"

namespace AccountMgr
{
    AccountOpResult CreateAccount(std::string username, std::string password)
    {
        if (utf8length(username) > MAX_ACCOUNT_STR)
            return AOR_NAME_TOO_LONG;                           // username's too long

        if (utf8length(password) > MAX_PASS_STR)
            return AOR_PASS_TOO_LONG;                           // password's too long

        Utf8ToUpperOnlyLatin(username);
        Utf8ToUpperOnlyLatin(password);

        if (GetId(username))
            return AOR_NAME_ALREADY_EXIST;                      // username does already exist

        auto [salt, verifier] = Warhead::Crypto::SRP6::MakeRegistrationData(username, password);

        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT);
        stmt->SetData(0, username);
        stmt->SetData(1, salt);
        stmt->SetData(2, verifier);
        stmt->SetData(3, sGameConfig->GetOption<uint8>("Expansion"));
        AuthDatabase.Execute(stmt);

        stmt = AuthDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS_INIT);
        AuthDatabase.Execute(stmt);

        return AOR_OK;                                          // everything's fine
    }

    AccountOpResult DeleteAccount(uint32 accountId)
    {
        // Check if accounts exists
        AuthDatabasePreparedStatement loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_ID);
        loginStmt->SetData(0, accountId);

        PreparedQueryResult result = AuthDatabase.Query(loginStmt);
        if (!result)
            return AOR_NAME_NOT_EXIST;

        // Obtain accounts characters
        CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARS_BY_ACCOUNT_ID);
        stmt->SetData(0, accountId);

        result = CharacterDatabase.Query(stmt);

        if (result)
        {
            do
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>((*result)[0].Get<uint32>());

                // Kick if player is online
                if (Player* p = ObjectAccessor::FindPlayer(guid))
                {
                    WorldSession* s = p->GetSession();
                    s->KickPlayer("Delete account");            // mark session to remove at next session list update
                    s->LogoutPlayer(false);                     // logout player without waiting next session list update
                }

                Player::DeleteFromDB(guid.GetCounter(), accountId, false, true);       // no need to update realm characters
            } while (result->NextRow());
        }

        // table realm specific but common for all characters of account for realm
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_TUTORIALS);
        stmt->SetData(0, accountId);
        CharacterDatabase.Execute(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ACCOUNT_DATA);
        stmt->SetData(0, accountId);
        CharacterDatabase.Execute(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_BAN);
        stmt->SetData(0, accountId);
        CharacterDatabase.Execute(stmt);

        AuthDatabaseTransaction trans = AuthDatabase.BeginTransaction();

        loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT);
        loginStmt->SetData(0, accountId);
        trans->Append(loginStmt);

        loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_ACCESS);
        loginStmt->SetData(0, accountId);
        trans->Append(loginStmt);

        loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS);
        loginStmt->SetData(0, accountId);
        trans->Append(loginStmt);

        loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_DEL_ACCOUNT_BANNED);
        loginStmt->SetData(0, accountId);
        trans->Append(loginStmt);

        loginStmt = AuthDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_MUTE_EXPIRED);
        loginStmt->SetData(0, accountId);
        trans->Append(loginStmt);

        AuthDatabase.CommitTransaction(trans);

        return AOR_OK;
    }

    AccountOpResult ChangeUsername(uint32 accountId, std::string newUsername, std::string newPassword)
    {
        // Check if accounts exists
        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_ID);
        stmt->SetData(0, accountId);
        PreparedQueryResult result = AuthDatabase.Query(stmt);

        if (!result)
            return AOR_NAME_NOT_EXIST;

        if (utf8length(newUsername) > MAX_ACCOUNT_STR)
            return AOR_NAME_TOO_LONG;

        if (utf8length(newPassword) > MAX_PASS_STR)
            return AOR_PASS_TOO_LONG;                           // password's too long

        Utf8ToUpperOnlyLatin(newUsername);
        Utf8ToUpperOnlyLatin(newPassword);

        stmt = AuthDatabase.GetPreparedStatement(LOGIN_UPD_USERNAME);
        stmt->SetData(0, newUsername);
        stmt->SetData(1, accountId);
        AuthDatabase.Execute(stmt);

        auto [salt, verifier] = Warhead::Crypto::SRP6::MakeRegistrationData(newUsername, newPassword);
        stmt = AuthDatabase.GetPreparedStatement(LOGIN_UPD_LOGON);
        stmt->SetData(0, salt);
        stmt->SetData(1, verifier);
        stmt->SetData(2, accountId);
        AuthDatabase.Execute(stmt);

        return AOR_OK;
    }

    AccountOpResult ChangePassword(uint32 accountId, std::string newPassword)
    {
        std::string username;

        if (!GetName(accountId, username))
        {
            sScriptMgr->OnFailedPasswordChange(accountId);
            return AOR_NAME_NOT_EXIST;                          // account doesn't exist
        }

        if (utf8length(newPassword) > MAX_PASS_STR)
        {
            sScriptMgr->OnFailedEmailChange(accountId);
            return AOR_PASS_TOO_LONG;                           // password's too long
        }

        Utf8ToUpperOnlyLatin(username);
        Utf8ToUpperOnlyLatin(newPassword);

        auto [salt, verifier] = Warhead::Crypto::SRP6::MakeRegistrationData(username, newPassword);

        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_UPD_LOGON);
        stmt->SetData(0, salt);
        stmt->SetData(1, verifier);
        stmt->SetData(2, accountId);
        AuthDatabase.Execute(stmt);

        sScriptMgr->OnPasswordChange(accountId);
        return AOR_OK;
    }

    uint32 GetId(std::string const& username)
    {
        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ID_BY_USERNAME);
        stmt->SetData(0, username);
        PreparedQueryResult result = AuthDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint32>() : 0;
    }

    uint32 GetSecurity(uint32 accountId)
    {
        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_GET_ACCOUNT_ACCESS_GMLEVEL);
        stmt->SetData(0, accountId);
        PreparedQueryResult result = AuthDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint8>() : uint32(SEC_PLAYER);
    }

    uint32 GetSecurity(uint32 accountId, int32 realmId)
    {
        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_GET_GMLEVEL_BY_REALMID);
        stmt->SetData(0, accountId);
        stmt->SetData(1, realmId);
        PreparedQueryResult result = AuthDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint8>() : uint32(SEC_PLAYER);
    }

    bool GetName(uint32 accountId, std::string& name)
    {
        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_GET_USERNAME_BY_ID);
        stmt->SetData(0, accountId);
        PreparedQueryResult result = AuthDatabase.Query(stmt);

        if (result)
        {
            name = (*result)[0].Get<std::string>();
            return true;
        }

        return false;
    }

    bool CheckPassword(uint32 accountId, std::string password)
    {
        std::string username;

        if (!GetName(accountId, username))
            return false;

        Utf8ToUpperOnlyLatin(username);
        Utf8ToUpperOnlyLatin(password);

        AuthDatabasePreparedStatement stmt = AuthDatabase.GetPreparedStatement(LOGIN_SEL_CHECK_PASSWORD);
        stmt->SetData(0, accountId);

        if (PreparedQueryResult result = AuthDatabase.Query(stmt))
        {
            Warhead::Crypto::SRP6::Salt salt = (*result)[0].Get<Binary, Warhead::Crypto::SRP6::SALT_LENGTH>();
            Warhead::Crypto::SRP6::Verifier verifier = (*result)[1].Get<Binary, Warhead::Crypto::SRP6::VERIFIER_LENGTH>();

            if (Warhead::Crypto::SRP6::CheckLogin(username, password, salt, verifier))
                return true;
        }

        return false;
    }

    uint32 GetCharactersCount(uint32 accountId)
    {
        // check character count
        CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
        stmt->SetData(0, accountId);
        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        return (result) ? (*result)[0].Get<uint64>() : 0;
    }

    bool IsPlayerAccount(uint32 gmlevel)
    {
        return gmlevel == SEC_PLAYER;
    }

    bool IsGMAccount(uint32 gmlevel)
    {
        return gmlevel >= SEC_MODERATOR && gmlevel <= SEC_CONSOLE;
    }

    bool IsAdminAccount(uint32 gmlevel)
    {
        return gmlevel >= SEC_ADMINISTRATOR && gmlevel <= SEC_CONSOLE;
    }

    bool IsConsoleAccount(uint32 gmlevel)
    {
        return gmlevel == SEC_CONSOLE;
    }

} // Namespace AccountMgr
