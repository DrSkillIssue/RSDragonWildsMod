#pragma once
#include "../Reflection.hpp"
#include "SpellWheel.hpp"
#include "Spells.hpp"
#include <array>

namespace Mods
{
struct Frame;

inline UObject* object_at(UObject* owner, int offset)
{
    UObject* result;
    std::memcpy(&result, reinterpret_cast<unsigned char*>(owner) + offset, sizeof(result));
    return result;
}

class Player
{
public:
    void bind(UObject* controller);
    void describe(Frame& frame);
    UObject* pawn(UObject* controller) const { return object_at(controller, m_pawn_offset); }
    UObject* identity() const { return m_identity; }
    SpellWheel& wheel() { return m_wheel; }
    Spells& spells() { return m_spells; }
    void reset();

private:
    UObject* m_identity{};
    UWorld* m_world{};
    UObject* m_outer{};
    int m_pawn_offset{};
    int m_input_mode_offset{};
    Flag m_cursor;
    UObject* m_gameplay_mode{};
    UObject* m_lock_on_mode{};
    UObject* m_radial_mode{};
    std::array<UObject*, 3> m_casting_modes{};
    SpellWheel m_wheel;
    Spells m_spells;
};
}
