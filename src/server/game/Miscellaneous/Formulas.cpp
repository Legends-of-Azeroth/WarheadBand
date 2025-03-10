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

#include "Formulas.h"
#include "Creature.h"
#include "GameConfig.h"
#include "Log.h"
#include "Player.h"

uint32 Warhead::XP::BaseGain(uint8 pl_level, uint8 mob_level, ContentLevels content)
{
    uint32 baseGain;
    uint32 nBaseExp;

    switch (content)
    {
    case CONTENT_1_60:
        nBaseExp = 45;
        break;
    case CONTENT_61_70:
        nBaseExp = 235;
        break;
    case CONTENT_71_80:
        nBaseExp = 580;
        break;
    default:
        LOG_ERROR("misc", "BaseGain: Unsupported content level {}", content);
        nBaseExp = 45;
        break;
    }

    if (mob_level >= pl_level)
    {
        uint8 nLevelDiff = mob_level - pl_level;
        if (nLevelDiff > 4)
            nLevelDiff = 4;

        baseGain = ((pl_level * 5 + nBaseExp) * (20 + nLevelDiff) / 10 + 1) / 2;
    }
    else
    {
        uint8 gray_level = GetGrayLevel(pl_level);
        if (mob_level > gray_level)
        {
            uint8 ZD = GetZeroDifference(pl_level);
            baseGain = (pl_level * 5 + nBaseExp) * (ZD + mob_level - pl_level) / ZD;
        }
        else
            baseGain = 0;
    }

    //sScriptMgr->OnBaseGainCalculation(baseGain, pl_level, mob_level, content); // pussywizard: optimization
    return baseGain;
}

uint32 Warhead::XP::Gain(Player* player, Unit* unit, bool isBattleGround /*= false*/)
{
    Creature* creature = unit->ToCreature();
    uint32 gain = 0;

    if (!creature || (!creature->IsTotem() && !creature->IsPet() && !creature->IsCritter() &&
        !(creature->GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_XP)))
    {
        float xpMod = 1.0f;

        gain = BaseGain(player->GetLevel(), unit->GetLevel(), GetContentLevelsForMapAndZone(unit->GetMapId(), unit->GetZoneId()));

        if (gain && creature)
        {
            if (creature->isElite())
            {
                // Elites in instances have a 2.75x XP bonus instead of the regular 2x world bonus.
                if (unit->GetMap() && unit->GetMap()->IsDungeon())
                    xpMod *= 2.75f;
                else
                    xpMod *= 2.0f;
            }

            xpMod *= creature->GetCreatureTemplate()->ModExperience;
        }

        if (isBattleGround)
        {
            switch (player->GetMapId())
            {
                case MAP_BG_ALTERAC_VALLEY:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillAV");
                    break;
                case MAP_BG_WARSONG_GULCH:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillWSG");
                    break;
                case MAP_BG_ARATHI_BASIN:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillAB");
                    break;
                case MAP_BG_EYE_OF_THE_STORM:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillEOTS");
                    break;
                case MAP_BG_STRAND_OF_THE_ANCIENTS:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillSOTA");
                    break;
                case MAP_BG_ISLE_OF_CONQUEST:
                    xpMod *= CONF_GET_FLOAT("Rate.XP.BattlegroundKillIC");
                    break;
            }
        }
        else
            xpMod *= CONF_GET_FLOAT("Rate.XP.Kill");

        // if players dealt less than 50% of the damage and were credited anyway (due to CREATURE_FLAG_EXTRA_NO_PLAYER_DAMAGE_REQ), scale XP gained appropriately (linear scaling)
        if (creature && creature->GetPlayerDamageReq())
        {
            xpMod *= 1.0f - 2.0f * creature->GetPlayerDamageReq() / creature->GetMaxHealth();
        }

        gain = uint32(gain * xpMod);
    }

    //sScriptMgr->OnGainCalculation(gain, player, u); // pussywizard: optimization
    return gain;
}