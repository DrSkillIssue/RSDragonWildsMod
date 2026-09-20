#include "Fake.hpp"
#include <cassert>
#include <cstring>
#include <cwchar>

namespace Fake
{
namespace
{
struct Text
{
    const wchar_t* characters;
    unsigned char reserved[8];
};

FProperty* field_named(UStruct* owner, const wchar_t* name)
{
    for (auto field = records.at(owner).children; field; field = records.at(field).next)
        if (records.at(field).name == name) return static_cast<FProperty*>(field);
    assert(!"The fake function has no parameter with this name");
    return nullptr;
}

struct Parameters
{
    UFunction* function;
    unsigned char* data;

    template<class T> Typed<T> at(const wchar_t* name) const
    {
        const auto& field = records.at(field_named(function, name));
        assert(field.size >= sizeof(T));
        return {data + field.offset};
    }

    template<class T> Typed<T> at(const wchar_t* name, const wchar_t* member) const
    {
        const auto& field = records.at(field_named(function, name));
        const auto& inner = records.at(field_named(field.structure, member));
        assert(inner.size == sizeof(T));
        return {data + field.offset + inner.offset};
    }
};

struct Handler
{
    const wchar_t* name;
    void (*run)(UObject* self, const Parameters& in);
};

void accepted(UObject*, const Parameters&) {}

void load_class(UObject*, const Parameters& in)
{
    ++game.class_loads;
    const auto found = game.loadable.find(game.imported_text);
    if (found == game.loadable.end()) { in.at<UObject*>(L"ReturnValue").write(nullptr); return; }
    game.paths.emplace(found->first, found->second);
    in.at<UObject*>(L"ReturnValue").write(found->second);
}

void load_asset(UObject*, const Parameters& in)
{
    ++game.asset_loads;
    if (auto object = in.at<UObject*>(L"Asset").read(); records.contains(object))
    {
        in.at<UObject*>(L"ReturnValue").write(object);
        return;
    }
    std::wstring path = game.imported_text;
    const auto last = path.find_last_of(L"/.");
    if (path[last] != L'.') path += L'.' + path.substr(last + 1);
    const auto found = game.paths.find(path);
    in.at<UObject*>(L"ReturnValue").write(found == game.paths.end() ? nullptr : found->second);
}

void create_widget(UObject*, const Parameters& in)
{
    auto type = reinterpret_cast<UClass*>(in.at<UObject*>(L"WidgetType").read());
    assert(type && in.at<UObject*>(L"OwningPlayer").read() == game.controller);
    const bool tooltip = records.at(type).name == L"WBP_TextOnlyTooltip_C";
    Object widget(tooltip ? L"Tooltip" : L"Row", type);
    auto& record = records.at(widget.pointer);
    record.outer = game.game_instance;
    record.collectable = true;
    for (auto field = records.at(type).children; field; field = records.at(field).next)
    {
        Object block(L"TooltipBlock");
        auto& child = records.at(block.pointer);
        child.outer = widget.pointer;
        child.collectable = true;
        std::memcpy(reinterpret_cast<unsigned char*>(widget.pointer) + records.at(field).offset, &block.pointer, sizeof(block.pointer));
    }
    (tooltip ? game.tooltips : game.rows).push_back(widget.pointer);
    in.at<UObject*>(L"ReturnValue").write(widget.pointer);
}

void set_tooltip(UObject* self, const Parameters& in)
{
    records.at(self).references.push_back(in.at<UObject*>(L"Widget").read());
}

void is_player_ready(UObject*, const Parameters& in) { in.at<bool>(L"ReturnValue").write(game.ready); }

void perks_for_skill(UObject*, const Parameters& in)
{
    assert(in.at<UObject*>(L"InSkillData").read() == game.skill);
    in.at<Array>(L"ReturnValue").write(Array{ game.perk_soft, game.perk_count, 3 });
}

void is_perk_unlocked(UObject*, const Parameters& in)
{
    in.at<bool>(L"ReturnValue").write(in.at<UObject*>(L"InSkillPerkData").read() != game.locked_perk);
}

void soft_reference(UObject*, const Parameters& in)
{
    auto& record = records.at(in.at<UObject*>(L"Object").read());
    if (!record.serial) record.serial = static_cast<int>(++game.serials);
}

void add_child(UObject* self, const Parameters& in)
{
    Object slot(L"Slot", widget_class);
    auto& record = records.at(slot.pointer);
    record.outer = self;
    record.collectable = true;
    record.references.push_back(in.at<UObject*>(L"Content").read());
    records.at(self).references.push_back(slot.pointer);
    in.at<UObject*>(L"ReturnValue").write(slot.pointer);
}

void add_to_viewport(UObject* self, const Parameters&)
{
    auto& record = records.at(self);
    assert(!record.attached);
    record.attached = true;
    ++game.adds;
}

void remove_from_parent(UObject* self, const Parameters&)
{
    records.at(self).attached = false;
    ++game.removes;
}

void set_width(UObject*, const Parameters& in) { game.last_width = in.at<float>(L"InWidthOverride").read(); }
void set_height(UObject*, const Parameters& in) { game.last_height = in.at<float>(L"InHeightOverride").read(); }
void set_text(UObject* self, const Parameters&) { records.at(self).display_text = game.imported_text; }
void set_visibility(UObject* self, const Parameters&) { records.at(self).visibility = game.imported_text; }
void set_brush_color(UObject* self, const Parameters&) { records.at(self).brush = game.imported_text; }

void set_brush_from_texture(UObject*, const Parameters& in)
{
    assert(in.at<UObject*>(L"Texture").read());
    ++game.style_textures;
}

void set_desired_size(UObject*, const Parameters& in)
{
    game.last_icon_x = in.at<double>(L"DesiredSize", L"X").read();
    game.last_icon_y = in.at<double>(L"DesiredSize", L"Y").read();
}

void set_render_scale(UObject*, const Parameters& in)
{
    assert(in.at<double>(L"Scale", L"X").read() == 1.0);
    game.last_scale_y = in.at<double>(L"Scale", L"Y").read();
}

void is_hovered(UObject* self, const Parameters& in)
{
    const bool root = records.at(self).roots != 0;
    in.at<bool>(L"ReturnValue").write(self == game.hovered_frame || (root && game.hovered_frame));
}

void owning_player(UObject* self, const Parameters& in)
{
    assert(self != game.spellbook_wheel && self != game.wheel_template);
    in.at<UObject*>(L"ReturnValue").write(game.controller);
}

void radial_changed(UObject*, const Parameters& in)
{
    ++game.page_changes;
    game.selected_page = in.at<uint16_t>(L"SelectedIndex").read();
}

void select_slice(UObject* self, const Parameters&)
{
    ++game.selections;
    assert(self == game.wheel);
    if (game.selection_allowed) game.current_slot = member(self, L"CachedSectionId").read<uint8_t>(L"ByteProperty");
}

void selected_spell(UObject*, const Parameters& in)
{
    in.at<UObject*>(L"ReturnValue").write(game.assigned_spells[game.selected_page * 12 + game.current_slot]);
}

void world_time(UObject*, const Parameters& in)
{
    assert(in.at<UObject*>(L"WorldContextObject").read() == game.controller);
    in.at<double>(L"ReturnValue").write(game.world_time);
}

void text_to_string(UObject*, const Parameters& in)
{
    const auto text = in.at<Text>(L"InText").read();
    const int count = static_cast<int>(std::wcslen(text.characters)) + 1;
    const Array out{reinterpret_cast<unsigned char*>(const_cast<wchar_t*>(text.characters)), count, count};
    in.at<Array>(L"ReturnValue").write(out);
}

constexpr Handler handlers[] = {
    {L"IsPlayerReady", is_player_ready},
    {L"GetPerksForSkill", perks_for_skill},
    {L"IsPerkUnlocked", is_perk_unlocked},
    {L"LoadClassAsset_Blocking", load_class},
    {L"LoadAsset_Blocking", load_asset},
    {L"Create", create_widget},
    {L"SetToolTip", set_tooltip},
    {L"Conv_ObjectToSoftObjectReference", soft_reference},
    {L"AddChildToCanvas", add_child},
    {L"SetContent", add_child},
    {L"AddChildToHorizontalBox", add_child},
    {L"AddChildToVerticalBox", add_child},
    {L"AddChild", add_child},
    {L"AddChildToWrapBox", add_child},
    {L"SetInnerSlotPadding", accepted},
    {L"AddToViewport", add_to_viewport},
    {L"RemoveFromParent", remove_from_parent},
    {L"SetWidthOverride", set_width},
    {L"SetHeightOverride", set_height},
    {L"SetText", set_text},
    {L"SetVisibility", set_visibility},
    {L"SetBrushColor", set_brush_color},
    {L"SetBrushFromTexture", set_brush_from_texture},
    {L"SetDesiredSizeOverride", set_desired_size},
    {L"SetRenderScale", set_render_scale},
    {L"IsHovered", is_hovered},
    {L"GetOwningPlayer", owning_player},
    {L"OnSpellRadialChanged", radial_changed},
    {L"SelectSlice", select_slice},
    {L"GetCurrentlySelectedSpellData", selected_spell},
    {L"GetTimeSeconds", world_time},
    {L"Conv_TextToString", text_to_string},
    {L"SetOwningPlayer", accepted},
    {L"SetVerticalAlignment", accepted},
    {L"SetHorizontalAlignment", accepted},
    {L"SetAnchors", accepted},
    {L"SetAlignment", accepted},
    {L"SetPosition", accepted},
    {L"SetAutoSize", accepted},
    {L"SetBrush", accepted},
    {L"SetBrushFromSoftTexture", accepted},
    {L"SetPadding", accepted},
    {L"SetColorAndOpacity", accepted},
    {L"SetShadowColorAndOpacity", accepted},
    {L"SetShadowOffset", accepted},
    {L"SetZOrder", accepted},
    {L"SetRenderTransformPivot", accepted},
    {L"SetSize", accepted},
    {L"SetScrollbarThickness", accepted},
    {L"Server_NotifyPreparingSpell", accepted},
    {L"Multicast_SendPayloadForSpellCasting", accepted},
    {L"ClientRestart", accepted},
    {L"HandleOnSpellCastingAnimationFinished", accepted},
};
}
}

namespace RC::Unreal
{
void UObject::ProcessEvent(UFunction* function, void* data)
{
    ++Fake::game.calls;
    assert(Fake::records.at(this).alive);
    const auto& name = Fake::records.at(function).name;
    bool handled = false;
    for (const auto& handler : Fake::handlers)
        if (name == handler.name) { handler.run(this, {function, static_cast<unsigned char*>(data)}); handled = true; break; }
    assert(handled && "The fake engine has no handler for this function");
    alignas(16) unsigned char frame[256]{};
    UnrealScriptFunctionCallableContext context{ this, *reinterpret_cast<FFrame*>(frame), nullptr };
    for (auto& [hook, custom] : Fake::records.at(function).post_hooks)
        if (hook) hook(context, custom);
}
}
