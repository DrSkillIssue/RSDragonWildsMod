#pragma once
#include "../Reflection.hpp"
#include <optional>
#include <string>

namespace Mods
{
struct Frame;

enum class Selection { requested, not_assigned, no_widget };

class SpellWheel
{
public:
    void bind(UObject* controller);
    bool ready(UObject* controller);
    Selection select(UObject* asset, const Frame& frame);
    std::wstring name(UObject* spell);
    void reset();

private:
    struct Widget
    {
        FWeakObjectPtr reference;
        Retained<std::uint16_t> page;
        Call select;
        Elements slices;
        Typed<std::uint8_t> section;
        explicit Widget(UObject* candidate);
    };
    int m_component_offset{};
    UObject* m_component{};
    Elements m_spells;
    Typed<std::uint8_t> m_pages;
    Typed<std::uint8_t> m_per_page;
    std::optional<Retained<UObject*>> m_selected;
    UClass* m_spell_class{};
    FProperty* m_name_field{};
    std::optional<Call> m_convert;
    Value m_convert_in{};
    Value m_convert_out{};
    std::optional<Widget> m_widget;

    int index_of(UObject* asset, const Array& array, int* copies = nullptr) const;
    void describe(UObject* spell);
    bool find_widget(const Frame& frame);
};
}
