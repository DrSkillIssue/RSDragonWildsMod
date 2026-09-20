#pragma once
#include "../Reflection.hpp"
#include <string>

namespace Mods::Widgets
{
enum Type { user_widget, widget_tree, canvas, invalidation_box, horizontal_box, size_box, border, text_block, image, vertical_box, scroll_box, wrap_box, type_count };

void resolve();
UObject* make(Type type, const std::wstring& name, UObject* outer);

struct Style
{
    UObject* font;
    float scale;
};

class Text
{
public:
    Text(UObject* tree, const std::wstring& name, const Style& style, float size, const wchar_t* color, bool shadow, int letter_spacing);
    UObject* block() const { return m_block; }
    void set_text(const std::wstring& value) { m_text.set(value); }
    void set_visibility(const wchar_t* state) { m_visibility.set(state); }

private:
    UObject* m_block;
    Retained<std::wstring> m_text;
    Retained<std::wstring> m_visibility;
};

}
