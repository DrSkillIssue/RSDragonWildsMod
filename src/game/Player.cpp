#include "Player.hpp"
#include "Frame.hpp"

namespace Mods
{
void Player::bind(UObject* controller)
{
    m_world = controller->GetWorld();
    if (!m_world) throw std::runtime_error("Player controller has no world");
    m_outer = member(reinterpret_cast<UObject*>(m_world), L"OwningGameInstance").read<UObject*>(L"ObjectProperty");
    if (!m_outer) throw std::runtime_error("World has no game instance");
    auto pawn_field = member(controller, L"Pawn");
    pawn_field.check(L"ObjectProperty", sizeof(UObject*));
    m_pawn_offset = pawn_field.field->GetOffset_ForInternal();
    auto mode_field = member(controller, L"CurrentInputMode");
    mode_field.check(L"ObjectProperty", sizeof(UObject*));
    m_input_mode_offset = mode_field.field->GetOffset_ForInternal();
    m_cursor = member(controller, L"bShowMouseCursor").flag();
    m_gameplay_mode = member(controller, L"GameplayInputMode").read<UObject*>(L"ObjectProperty");
    if (!m_gameplay_mode) throw std::runtime_error("Gameplay input mode is unavailable");
    static UObject unmatched;
    const wchar_t* optional_modes[] = { L"GameplayLockOnTargetingInputMode", L"RadialMenuInputMode",
        L"SpellcastingModeInputMode", L"SpellPlacementModeInputMode", L"SpellPlacementModeAdvancedMovementInputMode" };
    UObject** targets[] = { &m_lock_on_mode, &m_radial_mode, &m_casting_modes[0], &m_casting_modes[1], &m_casting_modes[2] };
    for (int i = 0; i < 5; ++i)
    {
        auto value = member(controller, optional_modes[i]).read<UObject*>(L"ObjectProperty");
        *targets[i] = value ? value : &unmatched;
    }
    m_wheel.bind(controller);
    m_spells.bind(controller);
    m_identity = controller;
}

void Player::describe(Frame& frame)
{
    auto mode = object_at(m_identity, m_input_mode_offset);
    frame.player = this;
    frame.controller = m_identity;
    frame.world = m_world;
    frame.outer = m_outer;
    frame.gameplay = pawn(m_identity) && (mode == m_gameplay_mode || mode == m_lock_on_mode);
    frame.cursor = m_cursor.read() && mode != m_radial_mode;
    frame.shown = frame.gameplay || frame.cursor || mode == m_casting_modes[0] || mode == m_casting_modes[1] || mode == m_casting_modes[2];
}

void Player::reset()
{
    m_identity = nullptr;
    m_world = nullptr;
    m_outer = nullptr;
    m_wheel.reset();
    m_spells.reset();
}
}
