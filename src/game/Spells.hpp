#pragma once
#include "../Reflection.hpp"
#include <optional>
#include <string>
#include <vector>

namespace Mods
{
class Spells
{
public:
    struct Entry
    {
        UObject* spell;
        std::wstring name;
        UObject* perk;
        bool needs_unlocking;
    };
    void bind(UObject* controller);
    UObject* find(const std::wstring& name) const;
    std::vector<const Entry*> unlocked();
    float cooldown(UObject* spell);
    void reset();

private:
    std::optional<Call> m_unlocked;
    std::vector<Entry> m_entries;
    std::vector<UObject*> m_perks;

    bool unlocked(UObject* perk);
};
}
