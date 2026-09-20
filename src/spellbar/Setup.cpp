#include "SpellBar.hpp"
#include "../Log.hpp"
#include <utility>

namespace Mods
{
void Setup::frame(Bar& bar, const Settings::SpellBar& config, const Frame& current)
{
    m_active = current.cursor;
    if (!m_active && bar.picker_open()) bar.close_picker();
    const auto click = current.presses & (left_click | right_click | middle_click);
    if (!m_active || !click) return;
    if (bar.picker_open())
    {
        auto spell = click & left_click ? bar.picked() : nullptr;
        bar.close_picker();
        if (!spell) return;
        m_edit = Edit{ m_picker_slot, spell->GetName() };
        DW_LOG_DEBUG("Panel picked {} for slot {}.", std::string(m_edit->spell.begin(), m_edit->spell.end()), m_picker_slot + 1);
        return;
    }
    if (click == left_click) return;
    DW_LOG_DEBUG("Panel click {}: bar hovered {}.", click & middle_click ? "middle" : "right", bar.hovered());
    if (!bar.hovered()) return;
    int slot = -1;
    for (int i = 0; i < static_cast<int>(config.slots.size()) && slot < 0; ++i)
        if (bar.slot(i).hovered()) slot = i;
    DW_LOG_DEBUG("Panel click on slot {}.", slot + 1);
    if (slot < 0) return;
    if (click & middle_click)
    {
        m_edit = Edit{ slot, {} };
        return;
    }
    m_picker_slot = slot;
    bar.open_picker(*current.player);
    DW_LOG_DEBUG("Panel opened for slot {}.", slot + 1);
}

std::optional<Edit> Setup::take_edit()
{
    return std::exchange(m_edit, std::nullopt);
}
}
