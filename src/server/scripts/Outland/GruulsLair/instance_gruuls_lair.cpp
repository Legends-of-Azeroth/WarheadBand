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

#include "InstanceScript.h"
#include "ScriptObject.h"
#include "gruuls_lair.h"
#include <sstream>

DoorData const doorData[] =
{
    { GO_MAULGAR_DOOR,  DATA_MAULGAR,   DOOR_TYPE_PASSAGE },
    { GO_GRUUL_DOOR,    DATA_GRUUL,     DOOR_TYPE_ROOM    },
    { 0,                0,              DOOR_TYPE_ROOM    } // END
};

MinionData const minionData[] =
{
    { NPC_MAULGAR,              DATA_MAULGAR },
    { NPC_KROSH_FIREHAND,       DATA_MAULGAR },
    { NPC_OLM_THE_SUMMONER,     DATA_MAULGAR },
    { NPC_KIGGLER_THE_CRAZED,   DATA_MAULGAR },
    { NPC_BLINDEYE_THE_SEER,    DATA_MAULGAR }
};

class instance_gruuls_lair : public InstanceMapScript
{
public:
    instance_gruuls_lair() : InstanceMapScript("instance_gruuls_lair", 565) { }

    struct instance_gruuls_lair_InstanceMapScript : public InstanceScript
    {
        instance_gruuls_lair_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);
            LoadDoorData(doorData);
            LoadMinionData(minionData);

            _addsKilled = 0;
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_MAULGAR:
                    _maulgarGUID = creature->GetGUID();
                    [[fallthrough]];
                case NPC_KROSH_FIREHAND:
                case NPC_OLM_THE_SUMMONER:
                case NPC_KIGGLER_THE_CRAZED:
                case NPC_BLINDEYE_THE_SEER:
                    AddMinion(creature, true);
                    break;
            }
        }

        void OnCreatureRemove(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_MAULGAR:
                case NPC_KROSH_FIREHAND:
                case NPC_OLM_THE_SUMMONER:
                case NPC_KIGGLER_THE_CRAZED:
                case NPC_BLINDEYE_THE_SEER:
                    AddMinion(creature, false);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_MAULGAR_DOOR:
                case GO_GRUUL_DOOR:
                    AddDoor(go, true);
                    break;
            }
        }

        void OnGameObjectRemove(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_MAULGAR_DOOR:
                case GO_GRUUL_DOOR:
                    AddDoor(go, false);
                    break;
            }
        }

        bool SetBossState(uint32 id, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            if (id == DATA_MAULGAR && state == NOT_STARTED)
                _addsKilled = 0;
            return true;
        }

        void SetData(uint32 type, uint32  /*id*/) override
        {
            if (type == DATA_ADDS_KILLED)
                if (Creature* maulgar = instance->GetCreature(_maulgarGUID))
                    maulgar->AI()->DoAction(++_addsKilled);
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == DATA_ADDS_KILLED)
                return _addsKilled;
            return 0;
        }

    protected:
        uint32 _addsKilled;
        ObjectGuid _maulgarGUID;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_gruuls_lair_InstanceMapScript(map);
    }
};

void AddSC_instance_gruuls_lair()
{
    new instance_gruuls_lair();
}