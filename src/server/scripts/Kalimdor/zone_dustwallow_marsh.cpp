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

/* ScriptData
SDName: Dustwallow_Marsh
SD%Complete: 95
SDComment: Quest support: 11180, 11126, 11174
SDCategory: Dustwallow Marsh
EndScriptData */

/* ContentData
npc_cassa_crimsonwing - handled by npc_taxi
EndContentData */

#include "Player.h"
#include "ScriptObject.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"

/*######
## npc_zelfrax
######*/

Position const MovePosition = {-2967.030f, -3872.1799f, 35.620f, 0.0f};

enum Zelfrax
{
    SAY_ZELFRAX1     = 0,
    SAY_ZELFRAX2     = 1
};

class npc_zelfrax : public CreatureScript
{
public:
    npc_zelfrax() : CreatureScript("npc_zelfrax") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_zelfraxAI(creature);
    }

    struct npc_zelfraxAI : public ScriptedAI
    {
        npc_zelfraxAI(Creature* creature) : ScriptedAI(creature)
        {
            MoveToDock();
        }

        void AttackStart(Unit* who) override
        {
            if (!who)
                return;

            if (me->Attack(who, true))
            {
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);

                if (IsCombatMovementAllowed())
                    me->GetMotionMaster()->MoveChase(who);
            }
        }

        void MovementInform(uint32 Type, uint32 /*Id*/) override
        {
            if (Type != POINT_MOTION_TYPE)
                return;

            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            me->SetImmuneToPC(false);
            SetCombatMovement(true);

            if (me->IsInCombat())
                if (Unit* unit = me->GetVictim())
                    me->GetMotionMaster()->MoveChase(unit);
        }

        void MoveToDock()
        {
            SetCombatMovement(false);
            me->GetMotionMaster()->MovePoint(0, MovePosition);
            Talk(SAY_ZELFRAX1);
            Talk(SAY_ZELFRAX2);
        }

        void UpdateAI(uint32 /*Diff*/) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

enum SpellScripts
{
    SPELL_OOZE_ZAP              = 42489,
    SPELL_OOZE_ZAP_CHANNEL_END  = 42485,
    SPELL_OOZE_CHANNEL_CREDIT   = 42486,
    SPELL_ENERGIZED             = 42492,
};

class spell_ooze_zap : public SpellScriptLoader
{
public:
    spell_ooze_zap() : SpellScriptLoader("spell_ooze_zap") { }

    class spell_ooze_zap_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ooze_zap_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_OOZE_ZAP });
        }

        SpellCastResult CheckRequirement()
        {
            if (!GetCaster()->HasAura(GetSpellInfo()->Effects[EFFECT_1].CalcValue()))
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW; // This is actually correct

            if (!GetExplTargetUnit())
                return SPELL_FAILED_BAD_TARGETS;

            return SPELL_CAST_OK;
        }

        void HandleDummy(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            if (GetHitUnit())
                GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnCheckCast += SpellCheckCastFn(spell_ooze_zap_SpellScript::CheckRequirement);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ooze_zap_SpellScript();
    }
};

class spell_ooze_zap_channel_end : public SpellScriptLoader
{
public:
    spell_ooze_zap_channel_end() : SpellScriptLoader("spell_ooze_zap_channel_end") { }

    class spell_ooze_zap_channel_end_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ooze_zap_channel_end_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_OOZE_ZAP_CHANNEL_END });
        }

        void HandleDummy(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            if (Player* player = GetCaster()->ToPlayer())
                player->CastSpell(player, SPELL_OOZE_CHANNEL_CREDIT, true);
            Unit::Kill(GetHitUnit(), GetHitUnit());
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_channel_end_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ooze_zap_channel_end_SpellScript();
    }
};

class spell_energize_aoe : public SpellScriptLoader
{
public:
    spell_energize_aoe() : SpellScriptLoader("spell_energize_aoe") { }

    class spell_energize_aoe_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_energize_aoe_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_ENERGIZED });
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
            {
                if ((*itr)->GetTypeId() == TYPEID_PLAYER && (*itr)->ToPlayer()->GetQuestStatus(GetSpellInfo()->Effects[EFFECT_1].CalcValue()) == QUEST_STATUS_INCOMPLETE)
                    ++itr;
                else
                    targets.erase(itr++);
            }
            targets.push_back(GetCaster());
        }

        void HandleScript(SpellEffIndex effIndex)
        {
            PreventHitDefaultEffect(effIndex);
            GetCaster()->CastSpell(GetCaster(), uint32(GetEffectValue()), true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_energize_aoe_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_energize_aoe_SpellScript();
    }
};

void AddSC_dustwallow_marsh()
{
    new npc_zelfrax();
    new spell_ooze_zap();
    new spell_ooze_zap_channel_end();
    new spell_energize_aoe();
}
