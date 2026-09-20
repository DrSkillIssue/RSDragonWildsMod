#include "Widgets.hpp"
#include <array>

namespace Mods::Widgets
{
namespace
{
std::array<UClass*, type_count> classes{};
constexpr const wchar_t* paths[type_count] = {
    L"/Script/UMG.UserWidget", L"/Script/UMG.WidgetTree", L"/Script/UMG.CanvasPanel",
    L"/Script/UMG.InvalidationBox", L"/Script/UMG.HorizontalBox", L"/Script/UMG.SizeBox",
    L"/Script/UMG.Border", L"/Script/UMG.TextBlock", L"/Script/UMG.Image", L"/Script/UMG.VerticalBox", L"/Script/UMG.ScrollBox", L"/Script/UMG.WrapBox"
};
}

void resolve()
{
    if (classes[type_count - 1]) return;
    for (int i = 0; i < type_count; ++i)
    {
        classes[i] = static_cast<UClass*>(find(paths[i]));
        if (!classes[i]) throw std::runtime_error("Required UMG class is unavailable");
    }
}

UObject* make(Type type, const std::wstring& name, UObject* outer)
{
    FStaticConstructObjectParameters parameters(classes[type], outer);
    parameters.name = FName(name.c_str(), add_name, nullptr);
    parameters.flags |= 0x40;
    auto result = UObjectGlobals::StaticConstructObject(parameters);
    if (!result) throw std::runtime_error("Widget construction failed");
    return result;
}

Text::Text(UObject* tree, const std::wstring& name, const Style& style, float size, const wchar_t* color, bool shadow, int letter_spacing)
    : m_block(make(text_block, name, tree)), m_text(m_block, L"SetText", L"InText"), m_visibility(m_block, L"SetVisibility", L"InVisibility")
{
    auto font = member(m_block, L"Font");
    font.member(L"FontObject").write(L"ObjectProperty", style.font);
    font.member(L"TypefaceFontName").text(style.font ? L"None" : L"Regular");
    font.member(L"Size").text(std::to_wstring(std::max(6, static_cast<int>(size * style.scale))));
    font.member(L"LetterSpacing").text(std::to_wstring(letter_spacing));
    set(m_block, L"SetColorAndOpacity", L"InColorAndOpacity", std::wstring(L"(SpecifiedColor=") + color + L",ColorUseRule=UseColor_Specified)");
    if (shadow)
    {
        set(m_block, L"SetShadowColorAndOpacity", L"InShadowColorAndOpacity", L"(R=0,G=0,B=0,A=0.85)");
        set(m_block, L"SetShadowOffset", L"InShadowOffset", L"(X=1,Y=1)");
    }
}
}
