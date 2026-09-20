#include "SpellWheel.hpp"
#include "Frame.hpp"
#include <vector>

namespace Mods
{
SpellWheel::Widget::Widget(UObject* candidate)
    : reference(weak(candidate)),
      page(candidate, L"OnSpellRadialChanged", L"SelectedIndex", L"UInt16Property"),
      select(candidate, L"SelectSlice"),
      slices(member(candidate, L"Slices").elements(L"ObjectProperty", sizeof(UObject*))),
      section(member(candidate, L"CachedSectionId").as<std::uint8_t>(L"ByteProperty")) {}

void SpellWheel::bind(UObject* controller)
{
    auto field = member(controller, L"SpellcastingComponent");
    field.check(L"ObjectProperty", sizeof(UObject*));
    m_component_offset = field.field->GetOffset_ForInternal();
    m_convert.emplace(find(L"/Script/Engine.Default__KismetTextLibrary"), L"Conv_TextToString");
    m_convert_in = (*m_convert)[L"InText"];
    m_convert_out = (*m_convert)[L"ReturnValue"];
    m_convert_out.check(L"StrProperty", sizeof(Array));
}

bool SpellWheel::ready(UObject* controller)
{
    auto object = object_at(controller, m_component_offset);
    if (!live(object)) return false;
    if (object != m_component)
    {
        m_spells = member(object, L"SelectedSpells").elements(L"ObjectProperty", sizeof(UObject*));
        m_pages = member(object, L"NumSpellRadials").as<std::uint8_t>(L"ByteProperty");
        m_per_page = member(object, L"NumSpellSlotsPerRadial").as<std::uint8_t>(L"ByteProperty");
        m_selected.emplace(object, L"GetCurrentlySelectedSpellData", L"ReturnValue", L"ObjectProperty");
        m_component = object;
    }
    return true;
}

int SpellWheel::index_of(UObject* asset, const Array& array, int* copies) const
{
    int first = -1, found = 0;
    for (int i = 0; i < array.count; ++i)
    {
        if (m_spells.at<UObject*>(array, i) != asset) continue;
        if (first < 0) first = i;
        ++found;
    }
    if (copies) *copies = found;
    return first;
}

Selection SpellWheel::select(UObject* asset, const Frame& frame)
{
    const auto array = m_spells.array();
    const int per_page = m_per_page.read();
    const int count = m_pages.read() * per_page;
    if (!count || array.count != count) throw std::runtime_error("Spell wheel layout is unavailable or changed");
    int copies = 0;
    const int assigned_at = index_of(asset, array, &copies);
    if (assigned_at < 0) return Selection::not_assigned;
    if (!find_widget(frame)) return Selection::no_widget;
    const auto local_slot = static_cast<std::uint8_t>(assigned_at % per_page);
    if (!live(m_widget->slices.at<UObject*>(m_widget->slices.array(), local_slot))) throw std::runtime_error("Spell wheel is not initialized for selection");
    if (copies != 1 || m_selected->get() != asset) m_widget->page.set(static_cast<std::uint16_t>(assigned_at / per_page));
    m_widget->section.write(local_slot);
    m_widget->select.run();
    return Selection::requested;
}

bool SpellWheel::find_widget(const Frame& frame)
{
    auto live_widget = m_widget ? m_widget->reference.Get() : nullptr;
    if (live_widget && live_widget->GetWorld() == frame.world) return true;
    m_widget.reset();
    std::vector<UObject*> candidates;
    UObjectGlobals::FindAllOf(L"WBP_SurvivalSorcery_RadialSelector_C", candidates);
    for (auto candidate : candidates)
    {
        if (!live(candidate) || candidate->GetWorld() != frame.world || member(candidate, L"bIsSpellbookInstance").boolean()) continue;
        Call owner(candidate, L"GetOwningPlayer"); owner.run();
        if (owner[L"ReturnValue"].read<UObject*>(L"ObjectProperty") != frame.controller) continue;
        m_widget.emplace(candidate);
        if (m_widget->slices.array().count == m_per_page.read()) return true;
        m_widget.reset();
    }
    return false;
}

void SpellWheel::describe(UObject* spell)
{
    if (spell->GetClassPrivate() == m_spell_class) return;
    auto display = member(spell, L"SpellDisplayName");
    display.check(L"TextProperty", m_convert_in.field->GetElementSize());
    m_name_field = display.field;
    m_spell_class = spell->GetClassPrivate();
}

std::wstring SpellWheel::name(UObject* spell)
{
    if (!spell) return L"";
    describe(spell);
    m_convert_in.field->CopyCompleteValue(m_convert_in.data, reinterpret_cast<unsigned char*>(spell) + m_name_field->GetOffset_ForInternal());
    m_convert->run();
    Array text;
    std::memcpy(&text, m_convert_out.data, sizeof(text));
    if (text.count < 2 || !text.data) return L"";
    return std::wstring(reinterpret_cast<const wchar_t*>(text.data), text.count - 1);
}

void SpellWheel::reset()
{
    m_component = nullptr;
    m_selected.reset();
    m_spell_class = nullptr;
    m_convert.reset();
    m_widget.reset();
}
}
