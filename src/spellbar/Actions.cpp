#include "SpellBar.hpp"
#include "../Log.hpp"
#include <bit>

namespace Mods
{
void SpellBar::sample(const Frame& frame)
{
    if (!m_bar || !frame.controller) return;
    if (m_cast_window)
    {
        m_cooldowns.watch(frame.now, m_cast_window);
        DW_LOG_DEBUG("Cast event received from the game, watching for {} ms.", m_cast_window);
        m_cast_window = 0;
    }
    m_display_dirty = m_cooldowns.poll(*m_bar, m_config, frame) || m_display_dirty || frame.shown != m_bar->visible() || frame.cursor != m_setup.active();
    const auto pressed = frame.presses & ((1u << m_config.slots.size()) - 1);
    if (!pressed || !frame.gameplay || frame.cursor) return;
    activate(std::countr_zero(pressed), frame);
}

void SpellBar::activate(int index, const Frame& frame)
{
    auto& player = *frame.player;
    if (!player.wheel().ready(frame.controller)) return;
    auto asset = m_bar->slot(index).asset();
    if (!live(asset)) asset = nullptr;
    switch (asset ? player.wheel().select(asset, frame) : Selection::not_assigned)
    {
    case Selection::not_assigned:
        Log::write("Spell is not loaded or not assigned to a spell wheel. Assign it in the spellbook first.");
        return;
    case Selection::no_widget:
        Log::write("The game's spell wheel is not ready. Open and close the spell wheel, then try again.");
        return;
    case Selection::requested:
        DW_LOG_DEBUG("Slot {}: spell selection sent to the game.", index + 1);
        return;
    }
}
}
