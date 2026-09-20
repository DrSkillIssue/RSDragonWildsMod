#include "Spells.hpp"
#include "../Log.hpp"
#include <algorithm>

namespace Mods
{
void Spells::bind(UObject* controller)
{
    auto component = member(controller, L"SkillPerkComponent").read<UObject*>(L"ObjectProperty");
    if (!component) throw std::runtime_error("Skill perk component is unavailable");
    m_unlocked.emplace(component, L"IsPerkUnlocked");
    std::vector<UObject*> skills;
    UObjectGlobals::FindAllOf(L"SkillData", skills);
    Call perks(component, L"GetPerksForSkill");
    Call load(Mods::find(L"/Script/Engine.Default__KismetSystemLibrary"), L"LoadAsset_Blocking");
    for (auto skill : skills)
    {
        if (!live(skill)) continue;
        perks[L"InSkillData"].write(L"ObjectProperty", skill);
        perks.run();
        const auto soft = perks[L"ReturnValue"].elements(L"SoftObjectProperty", load[L"Asset"].field->GetElementSize());
        const auto perk_array = soft.array();
        for (int i = 0; i < perk_array.count; ++i)
        {
            load[L"Asset"].field->CopyCompleteValue(load[L"Asset"].data, perk_array.data + static_cast<std::size_t>(i) * soft.stride);
            load.run();
            auto perk = load[L"ReturnValue"].read<UObject*>(L"ObjectProperty");
            if (!perk) continue;
            perk->SetRootSet();
            const auto modules = member(perk, L"PerkModules").elements(L"ObjectProperty", sizeof(UObject*));
            const auto module_array = modules.array();
            for (int m = 0; m < module_array.count; ++m)
            {
                auto module = modules.at<UObject*>(module_array, m);
                if (!module || !module->GetPropertyByNameInChain(L"SpellData")) continue;
                auto spell = member(module, L"SpellData").read<UObject*>(L"ObjectProperty");
                if (!spell) continue;
                auto name = spell->GetName();
                if (find(name)) continue;
                spell->SetRootSet();
                m_entries.push_back({ spell, std::move(name), perk, member(spell, L"bNeedsUnlocking").boolean() });
            }
            m_perks.push_back(perk);
        }
    }
    std::ranges::sort(m_entries, {}, &Entry::name);
    DW_LOG_DEBUG("Spell catalog: {} spells from {} skills.", m_entries.size(), skills.size());
    if (!Log::debug) return;
    for (const auto& entry : m_entries)
    {
        const auto perk = entry.perk->GetName();
        Log::write("Spell {} from perk {}: {}", std::string(entry.name.begin(), entry.name.end()), std::string(perk.begin(), perk.end()), unlocked(entry.perk) ? "unlocked" : "locked");
    }
}

bool Spells::unlocked(UObject* perk)
{
    (*m_unlocked)[L"InSkillPerkData"].write(L"ObjectProperty", perk);
    m_unlocked->run();
    return (*m_unlocked)[L"ReturnValue"].boolean();
}

UObject* Spells::find(const std::wstring& name) const
{
    for (const auto& entry : m_entries)
        if (entry.name == name) return entry.spell;
    return nullptr;
}

std::vector<const Spells::Entry*> Spells::unlocked()
{
    std::vector<const Entry*> spells;
    spells.reserve(m_entries.size());
    for (const auto& entry : m_entries)
        if (!entry.needs_unlocking || unlocked(entry.perk)) spells.push_back(&entry);
    return spells;
}

float Spells::cooldown(UObject* spell)
{
    auto modifier = member(spell, L"CooldownModifierPerk");
    auto perk = modifier.member(L"CooldownPerk").read<UObject*>(L"ObjectProperty");
    if (perk && unlocked(perk)) return modifier.member(L"ModifiedCooldown").read<float>(L"FloatProperty");
    return member(spell, L"CooldownDuration").read<float>(L"FloatProperty");
}

void Spells::reset()
{
    for (const auto& entry : m_entries) entry.spell->ClearRootSet();
    for (auto perk : m_perks) perk->ClearRootSet();
    m_entries.clear();
    m_perks.clear();
    m_unlocked.reset();
}
}
