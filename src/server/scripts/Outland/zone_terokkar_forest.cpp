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

#include "Group.h"
#include "Player.h"
#include "ScriptObject.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellAuras.h"
#include "SpellScript.h"

// Ours
enum fumping
{
    SPELL_SUMMON_SAND_GNOME1            = 39240,
    SPELL_SUMMON_SAND_GNOME3            = 39247,
    SPELL_SUMMON_MATURE_BONE_SIFTER1    = 39241,
    SPELL_SUMMON_MATURE_BONE_SIFTER3    = 39245,
    SPELL_SUMMON_HAISHULUD              = 39248,
};

class spell_q10930_big_bone_worm : public SpellScriptLoader
{
public:
    spell_q10930_big_bone_worm() : SpellScriptLoader("spell_q10930_big_bone_worm") { }

    class spell_q10930_big_bone_worm_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10930_big_bone_worm_SpellScript);

        void SetDest(SpellDestination& dest)
        {
            Position const offset = { 0.5f, 0.5f, 5.0f, 0.0f };
            dest.RelocateOffset(offset);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_q10930_big_bone_worm_SpellScript::SetDest, EFFECT_1, TARGET_DEST_CASTER);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10930_big_bone_worm_SpellScript();
    }

    class spell_q10930_big_bone_worm_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_q10930_big_bone_worm_AuraScript);

        void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                return;

            GetUnitOwner()->CastSpell(GetUnitOwner(), RAND(SPELL_SUMMON_HAISHULUD, SPELL_SUMMON_MATURE_BONE_SIFTER1, SPELL_SUMMON_MATURE_BONE_SIFTER3), true);
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_q10930_big_bone_worm_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_q10930_big_bone_worm_AuraScript();
    }
};

class spell_q10929_fumping : SpellScriptLoader
{
public:
    spell_q10929_fumping() : SpellScriptLoader("spell_q10929_fumping") { }

    class spell_q10929_fumping_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10929_fumping_SpellScript);

        void SetDest(SpellDestination& dest)
        {
            Position const offset = { 0.5f, 0.5f, 5.0f, 0.0f };
            dest.RelocateOffset(offset);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_q10929_fumping_SpellScript::SetDest, EFFECT_1, TARGET_DEST_CASTER);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10929_fumping_SpellScript();
    }

    class spell_q10929_fumping_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_q10929_fumping_AuraScript);

        void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                return;

            GetUnitOwner()->CastSpell(GetUnitOwner(), RAND(SPELL_SUMMON_SAND_GNOME1, SPELL_SUMMON_SAND_GNOME3, SPELL_SUMMON_MATURE_BONE_SIFTER1, SPELL_SUMMON_MATURE_BONE_SIFTER3), true);
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_q10929_fumping_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_q10929_fumping_AuraScript();
    }
};

class npc_greatfather_aldrimus : public CreatureScript
{
public:
    npc_greatfather_aldrimus() : CreatureScript("npc_greatfather_aldrimus") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_greatfather_aldrimusAI(creature);
    }

    struct npc_greatfather_aldrimusAI : public ScriptedAI
    {
        npc_greatfather_aldrimusAI(Creature* c) : ScriptedAI(c) {}

        bool CanBeSeen(Player const* player) override
        {
            return player->GetQuestStatus(10253) == QUEST_STATUS_REWARDED;
        }
    };
};

enum q10036Torgos
{
    NPC_TORGOS                  = 18707
};

class spell_q10036_torgos : public SpellScriptLoader
{
public:
    spell_q10036_torgos() : SpellScriptLoader("spell_q10036_torgos") { }

    class spell_q10036_torgos_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10036_torgos_SpellScript);

        void HandleSendEvent(SpellEffIndex  /*effIndex*/)
        {
            if (Creature* torgos = GetCaster()->FindNearestCreature(NPC_TORGOS, 100.0f, true))
                torgos->GetAI()->AttackStart(GetCaster());
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_q10036_torgos_SpellScript::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10036_torgos_SpellScript();
    }
};

enum eQ10923EvilDrawsNear
{
    SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL1      = 39256,
    SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL2      = 39257,
    SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL_AURA  = 39259,

    NPC_AUCHENAI_DEATH_SPIRIT               = 21967
};

class spell_q10923_evil_draws_near_summon : public SpellScriptLoader
{
public:
    spell_q10923_evil_draws_near_summon() : SpellScriptLoader("spell_q10923_evil_draws_near_summon") { }

    class spell_q10923_evil_draws_near_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10923_evil_draws_near_summon_SpellScript);

        void HandleSendEvent(SpellEffIndex  /*effIndex*/)
        {
            if (Creature* auchenai = GetCaster()->FindNearestCreature(NPC_AUCHENAI_DEATH_SPIRIT, 10.0f, true))
                auchenai->CastSpell(auchenai, SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL_AURA, true);
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_q10923_evil_draws_near_summon_SpellScript::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10923_evil_draws_near_summon_SpellScript();
    }
};

class spell_q10923_evil_draws_near_periodic : public SpellScriptLoader
{
public:
    spell_q10923_evil_draws_near_periodic() : SpellScriptLoader("spell_q10923_evil_draws_near_periodic") { }

    class spell_q10923_evil_draws_near_periodic_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_q10923_evil_draws_near_periodic_AuraScript);

        void HandlePeriodic(AuraEffect const*  /*aurEff*/)
        {
            PreventDefaultAction();
            GetUnitOwner()->CastSpell(GetUnitOwner(), RAND(SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL1, SPELL_DUSTIN_UNDEAD_DRAGON_VISUAL2), true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_q10923_evil_draws_near_periodic_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_q10923_evil_draws_near_periodic_AuraScript();
    }
};

class spell_q10923_evil_draws_near_visual : public SpellScriptLoader
{
public:
    spell_q10923_evil_draws_near_visual() : SpellScriptLoader("spell_q10923_evil_draws_near_visual") { }

    class spell_q10923_evil_draws_near_visual_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10923_evil_draws_near_visual_SpellScript);

        void SetDest(SpellDestination& dest)
        {
            // Adjust effect summon position
            Position const offset = { 0.0f, 0.0f, 20.0f, 0.0f };
            dest.RelocateOffset(offset);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_q10923_evil_draws_near_visual_SpellScript::SetDest, EFFECT_0, TARGET_DEST_CASTER_RADIUS);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10923_evil_draws_near_visual_SpellScript();
    }
};

class spell_q10898_skywing : public SpellScriptLoader
{
public:
    spell_q10898_skywing() : SpellScriptLoader("spell_q10898_skywing") { }

    class spell_q10898_skywing_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q10898_skywing_SpellScript);

        void SetDest(SpellDestination& dest)
        {
            // Adjust effect summon position
            Position const offset = { frand(-7.0f, 7.0f), frand(-7.0f, 7.0f), 11.0f, 0.0f };
            dest.Relocate(*GetCaster());
            dest.RelocateOffset(offset);
        }

        void Register() override
        {
            OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_q10898_skywing_SpellScript::SetDest, EFFECT_0, TARGET_DEST_CASTER_RANDOM);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_q10898_skywing_SpellScript();
    }
};

// Theirs
/*######
## npc_unkor_the_ruthless
######*/

enum UnkorTheRuthless
{
    SAY_SUBMIT                      = 0,

    FACTION_HOSTILE                 = 45,
    QUEST_DONTKILLTHEFATONE         = 9889,

    SPELL_PULVERIZE                 = 2676
};

class npc_unkor_the_ruthless : public CreatureScript
{
public:
    npc_unkor_the_ruthless() : CreatureScript("npc_unkor_the_ruthless") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_unkor_the_ruthlessAI(creature);
    }

    struct npc_unkor_the_ruthlessAI : public ScriptedAI
    {
        npc_unkor_the_ruthlessAI(Creature* creature) : ScriptedAI(creature) { }

        bool CanDoQuest;
        uint32 UnkorUnfriendly_Timer;
        uint32 Pulverize_Timer;

        void Reset() override
        {
            CanDoQuest = false;
            UnkorUnfriendly_Timer = 0;
            Pulverize_Timer = 3000;
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetFaction(FACTION_HOSTILE);
        }

        void JustEngagedWith(Unit* /*who*/) override { }

        void DoNice()
        {
            Talk(SAY_SUBMIT);
            me->SetFaction(FACTION_FRIENDLY);
            me->SetStandState(UNIT_STAND_STATE_SIT);
            me->RemoveAllAuras();
            me->GetThreatMgr().ClearAllThreat();
            me->CombatStop(true);
            UnkorUnfriendly_Timer = 60000;
        }

        void DamageTaken(Unit* done_by, uint32& damage, DamageEffectType, SpellSchoolMask) override
        {
            if (!done_by)
                return;

            Player* player = done_by->ToPlayer();
            if (player && me->HealthBelowPctDamaged(30, damage))
            {
                if (Group* group = player->GetGroup())
                {
                    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                    {
                        Player* groupie = itr->GetSource();
                        if (groupie && groupie->IsInMap(player) &&
                                groupie->GetQuestStatus(QUEST_DONTKILLTHEFATONE) == QUEST_STATUS_INCOMPLETE &&
                                groupie->GetReqKillOrCastCurrentCount(QUEST_DONTKILLTHEFATONE, 18260) == 10)
                        {
                            groupie->AreaExploredOrEventHappens(QUEST_DONTKILLTHEFATONE);
                            if (!CanDoQuest)
                                CanDoQuest = true;
                        }
                    }
                }
                else if (player->GetQuestStatus(QUEST_DONTKILLTHEFATONE) == QUEST_STATUS_INCOMPLETE &&
                         player->GetReqKillOrCastCurrentCount(QUEST_DONTKILLTHEFATONE, 18260) == 10)
                {
                    player->AreaExploredOrEventHappens(QUEST_DONTKILLTHEFATONE);
                    CanDoQuest = true;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (CanDoQuest)
            {
                if (!UnkorUnfriendly_Timer)
                {
                    //DoCast(me, SPELL_QUID9889);        //not using spell for now
                    DoNice();
                }
                else
                {
                    if (UnkorUnfriendly_Timer <= diff)
                    {
                        EnterEvadeMode();
                        return;
                    }
                    else UnkorUnfriendly_Timer -= diff;
                }
            }

            if (!UpdateVictim())
                return;

            if (Pulverize_Timer <= diff)
            {
                DoCast(me, SPELL_PULVERIZE);
                Pulverize_Timer = 9000;
            }
            else Pulverize_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

/*######
## npc_isla_starmane
######*/
enum IslaStarmaneData
{
    SAY_PROGRESS_1  = 0,
    SAY_PROGRESS_2  = 1,
    SAY_PROGRESS_3  = 2,
    SAY_PROGRESS_4  = 3,

    QUEST_EFTW_H    = 10052,
    QUEST_EFTW_A    = 10051,
    GO_CAGE         = 182794,
    SPELL_CAT       = 32447,

    EVENT_SPELL_WRATH               = 1,
    EVENT_SPELL_MOONFIRE            = 2,
    EVENT_SPELL_ENTANGLING_ROOTS    = 3,

    SPELL_WRATH                     = 9739,
    SPELL_MOONFIRE                  = 15798,
    SPELL_ENTANGLING_ROOTS          = 33844
};

class npc_isla_starmane : public CreatureScript
{
public:
    npc_isla_starmane() : CreatureScript("npc_isla_starmane") { }

    struct npc_isla_starmaneAI : public npc_escortAI
    {
        npc_isla_starmaneAI(Creature* creature) : npc_escortAI(creature) { }

        void WaypointReached(uint32 waypointId) override
        {
            Player* player = GetPlayerForEscort();
            if (!player)
                return;

            switch (waypointId)
            {
                case 0:
                    if (GameObject* Cage = me->FindNearestGameObject(GO_CAGE, 10))
                        Cage->SetGoState(GO_STATE_ACTIVE);
                    break;
                case 2:
                    Talk(SAY_PROGRESS_1, player);
                    break;
                case 5:
                    Talk(SAY_PROGRESS_2, player);
                    break;
                case 6:
                    Talk(SAY_PROGRESS_3, player);
                    break;
                case 29:
                    Talk(SAY_PROGRESS_4, player);
                    if (player->GetTeamId() == TEAM_ALLIANCE)
                        player->GroupEventHappens(QUEST_EFTW_A, me);
                    else if (player->GetTeamId() == TEAM_HORDE)
                        player->GroupEventHappens(QUEST_EFTW_H, me);
                    me->SetInFront(player);
                    break;
                case 30:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                    break;
                case 31:
                    DoCast(me, SPELL_CAT);
                    me->SetWalk(false);
                    break;
            }
        }

        void Reset() override
        {
            me->RestoreFaction();
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Player* player = GetPlayerForEscort())
            {
                if (player->GetTeamId() == TEAM_ALLIANCE)
                    player->FailQuest(QUEST_EFTW_A);
                else if (player->GetTeamId() == TEAM_HORDE)
                    player->FailQuest(QUEST_EFTW_H);
            }
        }

        void JustEngagedWith(Unit*) override
        {
            events.Reset();
            events.ScheduleEvent(EVENT_SPELL_WRATH, 0);
            events.ScheduleEvent(EVENT_SPELL_MOONFIRE, 4000);
            events.ScheduleEvent(EVENT_SPELL_ENTANGLING_ROOTS, 10000);
        }

        void UpdateEscortAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_SPELL_WRATH:
                    me->CastSpell(me->GetVictim(), SPELL_WRATH, false);
                    events.ScheduleEvent(EVENT_SPELL_WRATH, 3000);
                    break;
                case EVENT_SPELL_MOONFIRE:
                    me->CastSpell(me->GetVictim(), SPELL_MOONFIRE, false);
                    events.ScheduleEvent(EVENT_SPELL_MOONFIRE, 12000);
                    break;
                case EVENT_SPELL_ENTANGLING_ROOTS:
                    me->CastSpell(me->GetVictim(), SPELL_ENTANGLING_ROOTS, false);
                    events.ScheduleEvent(EVENT_SPELL_ENTANGLING_ROOTS, 20000);
                    break;
            }

            DoMeleeAttackIfReady();
        }

        EventMap events;
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_EFTW_H || quest->GetQuestId() == QUEST_EFTW_A)
        {
            CAST_AI(npc_escortAI, (creature->AI()))->Start(true, false, player->GetGUID());
            creature->SetFaction(FACTION_ESCORTEE_N_NEUTRAL_ACTIVE);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_isla_starmaneAI(creature);
    }
};

/*######
## go_skull_pile
######*/
#define GOSSIP_S_DARKSCREECHER_AKKARAI         "Summon Darkscreecher Akkarai"
#define GOSSIP_S_KARROG         "Summon Karrog"
#define GOSSIP_S_GEZZARAK_THE_HUNTRESS         "Summon Gezzarak the Huntress"
#define GOSSIP_S_VAKKIZ_THE_WINDRAGER         "Summon Vakkiz the Windrager"

class go_skull_pile : public GameObjectScript
{
public:
    go_skull_pile() : GameObjectScript("go_skull_pile") { }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        ClearGossipMenuFor(player);
        switch (sender)
        {
            case GOSSIP_SENDER_MAIN:
                SendActionMenu(player, go, action);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if ((player->GetQuestStatus(11885) == QUEST_STATUS_INCOMPLETE) || player->GetQuestRewardStatus(11885))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_S_DARKSCREECHER_AKKARAI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_S_KARROG, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_S_GEZZARAK_THE_HUNTRESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_S_VAKKIZ_THE_WINDRAGER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }

        SendGossipMenuFor(player, go->GetGOInfo()->questgiver.gossipID, go->GetGUID());
        return true;
    }

    void SendActionMenu(Player* player, GameObject* /*go*/, uint32 action)
    {
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                player->CastSpell(player, 40642, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                player->CastSpell(player, 40640, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                player->CastSpell(player, 40632, false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                player->CastSpell(player, 40644, false);
                break;
        }
    }
};

/*######
## npc_slim
######*/

enum Slim
{
    FACTION_CONSORTIUM  = 933
};

class npc_slim : public CreatureScript
{
public:
    npc_slim() : CreatureScript("npc_slim") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        ClearGossipMenuFor(player);
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsVendor() && player->GetReputationRank(FACTION_CONSORTIUM) >= REP_FRIENDLY)
        {
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
            SendGossipMenuFor(player, 9896, creature->GetGUID());
        }
        else
            SendGossipMenuFor(player, 9895, creature->GetGUID());

        return true;
    }
};

void AddSC_terokkar_forest()
{
    // Ours
    new spell_q10930_big_bone_worm();
    new spell_q10929_fumping();
    new npc_greatfather_aldrimus();
    new spell_q10036_torgos();
    new spell_q10923_evil_draws_near_summon();
    new spell_q10923_evil_draws_near_periodic();
    new spell_q10923_evil_draws_near_visual();
    new spell_q10898_skywing();

    // Theirs
    new npc_unkor_the_ruthless();
    new npc_isla_starmane();
    new go_skull_pile();
    new npc_slim();
}
