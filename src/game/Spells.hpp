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
    void bind(UObject* controller);
    UObject* find(const std::wstring& name) const;
    std::vector<UObject*> unlocked();
    float cooldown(UObject* spell);
    void reset();

private:
    struct Entry
    {
        UObject* spell;
        std::wstring name;
        UObject* perk;
        bool needs_unlocking;
    };
    std::optional<Call> m_unlocked;
    std::vector<Entry> m_entries;

    bool unlocked(UObject* perk);
};
}
