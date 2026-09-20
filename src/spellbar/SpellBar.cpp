#include "SpellBar.hpp"
#include "../Log.hpp"
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <string>

namespace Mods
{
namespace
{
constexpr const wchar_t* plain = L"(R=1,G=1,B=1,A=1)";
constexpr const wchar_t* visibility_states[2][2] = { { L"Hidden", L"Hidden" }, { L"HitTestInvisible", L"Visible" } };
constexpr const wchar_t* tooltip_class_path = L"/Game/UI/Common/WBP_TextOnlyTooltip.WBP_TextOnlyTooltip_C";
constexpr const wchar_t* spell_slot_class_path = L"/Game/UI/InGameMenus/TopNavScreens/SpellBook/WBP_SpellSlot.WBP_SpellSlot_C";
constexpr const wchar_t* tooltip_box = L"(DrawAs=Box,Margin=(Left=0.08,Top=0.08,Right=0.08,Bottom=0.08),ResourceObject=/Script/Engine.Texture2D'/Game/Art/UI/Common/T_ToolTip_Bg.T_ToolTip_Bg',TintColor=(SpecifiedColor=(R=1,G=1,B=1,A=1)))";
constexpr const wchar_t* tooltip_line = L"(DrawAs=Image,ImageSize=(X=186,Y=1),ResourceObject=/Script/Engine.Texture2D'/Game/Art/UI/Building/Tooltip/T_Build_Repair_ToolTip_Line.T_Build_Repair_ToolTip_Line',TintColor=(SpecifiedColor=(R=1,G=1,B=1,A=1)))";
constexpr const wchar_t* tooltip_text = L"(R=0.730461,G=0.665387,B=0.584078,A=1)";
constexpr const wchar_t* action_lines = L"Right click: Choose spell\nMiddle click: Clear spell";
struct CastEvent { const wchar_t* function; std::uint64_t window; };
constexpr CastEvent cast_events[3] = {
    { L"/Script/Dominion.PlayerUtilityMagicComponent:Server_NotifyPreparingSpell", 10000 },
    { L"/Script/Dominion.PlayerMagicComponent:Multicast_SendPayloadForSpellCasting", 10000 },
    { L"/Script/Dominion.PlayerMagicComponent:HandleOnSpellCastingAnimationFinished", 1000 },
};

void center(UObject* layers, UObject* child, const wchar_t* z_order)
{
    auto slot = attach(layers, L"AddChildToCanvas", child);
    set(slot, L"SetAnchors", L"InAnchors", L"(Minimum=(X=0.5,Y=0.5),Maximum=(X=0.5,Y=0.5))");
    set(slot, L"SetAlignment", L"InAlignment", L"(X=0.5,Y=0.5)");
    set(slot, L"SetAutoSize", L"InbAutoSize", L"True");
    set(slot, L"SetZOrder", L"InZOrder", z_order);
}

UObject* game_class(const wchar_t* path)
{
    if (auto loaded = find(path)) return loaded;
    Call load(find(L"/Script/Engine.Default__KismetSystemLibrary"), L"LoadClassAsset_Blocking");
    load[L"AssetClass"].text(path);
    load.run();
    auto loaded = load[L"ReturnValue"].read<UObject*>(L"ClassProperty");
    if (!loaded) throw std::runtime_error("A game widget class is unavailable: " + narrow(path));
    return loaded;
}

UObject* game_widget(UObject* widget_class, UObject* owner)
{
    Call create(find(L"/Script/UMG.Default__WidgetBlueprintLibrary"), L"Create");
    create[L"WorldContextObject"].write(L"ObjectProperty", owner);
    create[L"WidgetType"].write(L"ClassProperty", widget_class);
    create[L"OwningPlayer"].write(L"ObjectProperty", owner);
    create.run();
    auto widget = create[L"ReturnValue"].read<UObject*>(L"ObjectProperty");
    if (!widget) throw std::runtime_error("A game widget could not be created");
    return widget;
}

void dump(UObject* widget, int depth)
{
    const std::string indent(static_cast<std::size_t>(depth) * 2, ' ');
    Log::write("{}{} {}", indent, narrow(widget->GetClassPrivate()->GetName().c_str()), narrow(widget->GetName().c_str()));
    for (auto property_name : { L"Background", L"Brush", L"Font", L"ColorAndOpacity", L"BrushColor", L"ContentColorAndOpacity", L"Padding", L"Text", L"WidgetStyle", L"Style",
        L"NormalBase", L"NormalHovered", L"NormalPressed", L"SelectedBase", L"SelectedHovered", L"SelectedPressed", L"DisabledBase", L"ButtonPadding", L"CustomPadding", L"NormalTextStyle", L"NormalHoveredTextStyle", L"SelectedTextStyle", L"Color", L"LineHeightPercentage", L"ShadowOffset", L"ShadowColor" })
    {
        auto field = widget->GetPropertyByNameInChain(property_name);
        if (!field) continue;
        FString text;
        field->ExportTextItem_Direct(text, reinterpret_cast<unsigned char*>(widget) + field->GetOffset_ForInternal(), nullptr, widget, 0, nullptr);
        Log::write("{}  {} = {}", indent, narrow(property_name), text.data ? narrow(text.data) : std::string());
        if (field->GetClass().GetFName() != FName(L"ClassProperty", find_name, nullptr)) continue;
        auto style_class = Typed<UClass*>{ reinterpret_cast<unsigned char*>(widget) + field->GetOffset_ForInternal() }.read();
        if (style_class && style_class->GetClassDefaultObject().value) dump(style_class->GetClassDefaultObject().value, depth + 1);
    }
    if (widget->GetPropertyByNameInChain(L"WidgetTree"))
    {
        auto tree = member(widget, L"WidgetTree").read<UObject*>(L"ObjectProperty");
        auto root = tree ? member(tree, L"RootWidget").read<UObject*>(L"ObjectProperty") : nullptr;
        if (root) dump(root, depth + 1);
    }
    if (!widget->GetPropertyByNameInChain(L"Slots")) return;
    const auto slots = member(widget, L"Slots").elements(L"ObjectProperty", sizeof(UObject*));
    const auto array = slots.array();
    for (int i = 0; i < array.count; ++i)
    {
        auto slot = slots.at<UObject*>(array, i);
        auto content = slot ? member(slot, L"Content").read<UObject*>(L"ObjectProperty") : nullptr;
        if (content) dump(content, depth + 1);
    }
}

std::wstring upper(std::wstring text)
{
    for (auto& c : text) if (c < 128) c = static_cast<wchar_t>(std::towupper(c));
    return text;
}
}

Slot::Slot(UObject* tree, UObject* panel, const Settings::SpellBar& config, int index, const std::wstring& suffix, const Widgets::Style& style, UObject* background, UObject* tooltip_class, UObject* owner, Player& player)
    : m_asset(player.spells().find(config.slots[index].spell)),
      m_border(Widgets::make(Widgets::border, L"SpellBarFrame" + suffix, tree)),
      m_hovered(m_border, L"IsHovered", L"ReturnValue"),
      m_layers(Widgets::make(Widgets::canvas, L"SpellBarLayers" + suffix, tree)),
      m_shade(Widgets::make(Widgets::image, L"SpellBarShade" + suffix, tree)),
      m_shade_visibility(m_shade, L"SetVisibility", L"InVisibility"),
      m_shade_scale(m_shade, L"SetRenderScale", L"Scale", L"Y", L"DoubleProperty"),
      m_seconds(tree, L"SpellBarSeconds" + suffix, style, 16, plain, true, 0)
{
    const auto icon_size = 40.0 * style.scale;
    auto size = Widgets::make(Widgets::size_box, L"SpellBarSize" + suffix, tree);
    Call width(size, L"SetWidthOverride");
    width[L"InWidthOverride"].write(L"FloatProperty", 52 * style.scale); width.run();
    Call height(size, L"SetHeightOverride");
    height[L"InHeightOverride"].write(L"FloatProperty", 52 * style.scale); height.run();
    auto cell = attach(panel, L"AddChildToHorizontalBox", size);
    const auto gap = index + 1 < static_cast<int>(config.slots.size()) ? static_cast<int>(16 * style.scale) : 0;
    set(cell, L"SetPadding", L"InPadding", L"(Left=0,Top=0,Right=" + std::to_wstring(gap) + L",Bottom=0)");
    if (background)
    {
        Call brush(m_border, L"SetBrushFromTexture");
        brush[L"Texture"].write(L"ObjectProperty", background); brush.run();
        set(m_border, L"SetBrushColor", L"InBrushColor", plain);
    }
    else set(m_border, L"SetBrush", L"InBrush", L"(DrawAs=Image,TintColor=(SpecifiedColor=(R=0.075,G=0.075,B=0.075,A=0.65),ColorUseRule=UseColor_Specified))");
    set(m_border, L"SetPadding", L"InPadding", L"(Left=0,Top=0,Right=0,Bottom=0)");
    attach(size, L"SetContent", m_border);
    attach(m_border, L"SetContent", m_layers);
    Widgets::Text label(tree, L"SpellBarKey" + suffix, style, 11, L"(R=0.9,G=0.86,B=0.77,A=1)", true, 0);
    std::wstring label_text = config.slots[index].key;
    if (!config.slots[index].spell.empty() && !m_asset) label_text += L" ?";
    label.set_text(label_text);
    auto key_slot = attach(m_layers, L"AddChildToCanvas", label.block());
    set(key_slot, L"SetPosition", L"InPosition", L"(X=" + std::to_wstring(static_cast<int>(3 * style.scale)) + L",Y=1)");
    set(key_slot, L"SetAutoSize", L"InbAutoSize", L"True");
    set(key_slot, L"SetZOrder", L"InZOrder", L"1");
    if (m_asset)
    {
        auto icon = Widgets::make(Widgets::image, L"SpellBarIcon" + suffix, tree);
        Call dimensions(icon, L"SetDesiredSizeOverride");
        dimensions[L"DesiredSize"].member(L"X").write(L"DoubleProperty", icon_size);
        dimensions[L"DesiredSize"].member(L"Y").write(L"DoubleProperty", icon_size);
        dimensions.run();
        auto source = member(m_asset, L"SpellIcon");
        Call brush(icon, L"SetBrushFromSoftTexture");
        auto destination = brush[L"SoftTexture"];
        source.check(L"SoftObjectProperty", destination.field->GetElementSize());
        destination.field->CopyCompleteValue(destination.data, source.data);
        brush.run();
        center(m_layers, icon, L"0");
    }
    Call cover_size(m_shade, L"SetDesiredSizeOverride");
    cover_size[L"DesiredSize"].member(L"X").write(L"DoubleProperty", icon_size);
    cover_size[L"DesiredSize"].member(L"Y").write(L"DoubleProperty", icon_size);
    cover_size.run();
    set(m_shade, L"SetBrush", L"InBrush", L"(DrawAs=Image,TintColor=(SpecifiedColor=(R=0,G=0,B=0,A=0.7),ColorUseRule=UseColor_Specified))");
    set(m_shade, L"SetRenderTransformPivot", L"Pivot", L"(X=0.5,Y=1)");
    m_shade_visibility.set(L"Collapsed");
    m_shade_scale.call[L"Scale"].member(L"X").write(L"DoubleProperty", 1.0);
    center(m_layers, m_shade, L"2");
    m_seconds.set_visibility(L"Collapsed");
    center(m_layers, m_seconds.block(), L"3");
    const auto name = player.wheel().name(m_asset);
    auto tooltip = game_widget(tooltip_class, owner);
    set(member(tooltip, L"Title").read<UObject*>(L"ObjectProperty"), L"SetText", L"InText", name.empty() ? L"EMPTY SLOT" : upper(name));
    std::wstring lines;
    if (!name.empty()) lines = L"Cooldown " + std::to_wstring(static_cast<int>(player.spells().cooldown(m_asset) + 0.5f)) + L" s\n";
    if (config.slots[index].key.empty()) lines += L"No key\n";
    else lines += L"Key: " + config.slots[index].key + L"\n";
    set(member(tooltip, L"Text").read<UObject*>(L"ObjectProperty"), L"SetText", L"InText", lines + action_lines);
    Call attach_tooltip(m_border, L"SetToolTip");
    attach_tooltip[L"Widget"].write(L"ObjectProperty", tooltip);
    attach_tooltip.run();
    if (Log::debug && index == 0) dump(tooltip, 0);
}

void Slot::show_cooldown(bool on)
{
    const wchar_t* state = on ? L"HitTestInvisible" : L"Collapsed";
    m_shade_visibility.set(state);
    m_seconds.set_visibility(state);
    m_shown_seconds = -1;
}

void Slot::set_cooldown(double scale, int seconds)
{
    m_shade_scale.set(scale);
    if (seconds == m_shown_seconds) return;
    m_shown_seconds = seconds;
    m_seconds.set_text(std::to_wstring(seconds));
}

Bar::Bar(const Settings::SpellBar& config, UObject* outer, UObject* owner, Player& player, std::uint64_t generation)
    : m_root(Widgets::make(Widgets::user_widget, L"DragonwildsSpellBar_" + std::to_wstring(generation), outer)),
      m_root_reference(weak(m_root)),
      m_detach(m_root, L"RemoveFromParent"),
      m_visibility(m_root, L"SetVisibility", L"InVisibility"),
      m_hovered(m_root, L"IsHovered", L"ReturnValue"),
      m_tree(Widgets::make(Widgets::widget_tree, L"SpellBarTree", m_root)),
      m_canvas(Widgets::make(Widgets::canvas, L"SpellBarCanvas", m_tree)),
      m_style{find(L"/Game/UI/Fonts/Poppins-Regular_Font.Poppins-Regular_Font"), config.scale},
      m_owner(owner)
{
    m_root->SetRootSet();
    if (!m_root->IsRootSet()) throw std::runtime_error("Widget root reference was not retained");
    show(false, false);
    member(m_root, L"bIsFocusable").text(L"False");
    member(m_root, L"TickFrequency").text(L"Never");
    Call owning(m_root, L"SetOwningPlayer");
    owning[L"LocalPlayerController"].write(L"ObjectProperty", owner); owning.run();
    member(m_root, L"WidgetTree").write(L"ObjectProperty", m_tree);
    member(m_tree, L"RootWidget").write(L"ObjectProperty", m_canvas);
    auto cache = Widgets::make(Widgets::invalidation_box, L"SpellBarCache", m_tree);
    auto cache_slot = attach(m_canvas, L"AddChildToCanvas", cache);
    set(cache_slot, L"SetAnchors", L"InAnchors", L"(Minimum=(X=0.5,Y=1),Maximum=(X=0.5,Y=1))");
    set(cache_slot, L"SetAlignment", L"InAlignment", L"(X=0.5,Y=1)");
    set(cache_slot, L"SetPosition", L"InPosition", L"(X=0,Y=-165)");
    set(cache_slot, L"SetAutoSize", L"InbAutoSize", L"True");
    auto panel = Widgets::make(Widgets::horizontal_box, L"SpellBarSlots", m_tree);
    attach(cache, L"SetContent", panel);
    auto background = find(L"/Game/Art/UI/Common/T_Common_ItemSlotsBackground.T_Common_ItemSlotsBackground");
    auto tooltips = game_class(tooltip_class_path);
    m_slots.reserve(config.slots.size());
    for (int i = 0; i < static_cast<int>(config.slots.size()); ++i) m_slots.emplace_back(m_tree, panel, config, i, std::to_wstring(i), m_style, background, tooltips, owner, player);
    set(m_root, L"AddToViewport", L"ZOrder", L"50");
    Log::write("Spell bar built.");
}

void Bar::open_picker(Player& player)
{
    m_picker_shown = true;
    if (m_picker_visibility)
    {
        m_picker_visibility->set(L"Visible");
        return;
    }
    auto size = Widgets::make(Widgets::size_box, L"SpellBarPickerSize", m_tree);
    Call width(size, L"SetWidthOverride");
    width[L"InWidthOverride"].write(L"FloatProperty", 360 * m_style.scale); width.run();
    Call height(size, L"SetHeightOverride");
    height[L"InHeightOverride"].write(L"FloatProperty", 330 * m_style.scale); height.run();
    auto frame = Widgets::make(Widgets::border, L"SpellBarPickerFrame", m_tree);
    set(frame, L"SetBrush", L"InBrush", tooltip_box);
    set(frame, L"SetPadding", L"InPadding", L"(Left=22,Top=18,Right=22,Bottom=22)");
    attach(size, L"SetContent", frame);
    auto column = Widgets::make(Widgets::vertical_box, L"SpellBarPickerColumn", m_tree);
    attach(frame, L"SetContent", column);
    const Widgets::Style title_style{ find(L"/Game/UI/Fonts/Amiri-Regular_Font.Amiri-Regular_Font"), m_style.scale };
    Widgets::Text title(m_tree, L"SpellBarPickerTitle", title_style, 10, tooltip_text, false, 200);
    title.set_text(L"CHOOSE SPELL");
    attach(column, L"AddChildToVerticalBox", title.block());
    auto separator = Widgets::make(Widgets::image, L"SpellBarPickerLine", m_tree);
    set(separator, L"SetBrush", L"InBrush", tooltip_line);
    auto separator_cell = attach(column, L"AddChildToVerticalBox", separator);
    set(separator_cell, L"SetPadding", L"InPadding", L"(Left=0,Top=4,Right=0,Bottom=6)");
    auto list = Widgets::make(Widgets::scroll_box, L"SpellBarPickerList", m_tree);
    Call thickness(list, L"SetScrollbarThickness");
    thickness[L"NewScrollbarThickness"].member(L"X").write(L"DoubleProperty", 4.0);
    thickness[L"NewScrollbarThickness"].member(L"Y").write(L"DoubleProperty", 4.0);
    thickness.run();
    auto list_size = Widgets::make(Widgets::size_box, L"SpellBarPickerListSize", m_tree);
    Call list_height(list_size, L"SetHeightOverride");
    list_height[L"InHeightOverride"].write(L"FloatProperty", 260 * m_style.scale); list_height.run();
    attach(list_size, L"SetContent", list);
    attach(column, L"AddChildToVerticalBox", list_size);
    auto grid = Widgets::make(Widgets::wrap_box, L"SpellBarPickerGrid", m_tree);
    Call spacing(grid, L"SetInnerSlotPadding");
    spacing[L"InPadding"].member(L"X").write(L"DoubleProperty", 6.0 * m_style.scale);
    spacing[L"InPadding"].member(L"Y").write(L"DoubleProperty", 6.0 * m_style.scale);
    spacing.run();
    attach(list, L"AddChild", grid);
    const auto spells = player.spells().unlocked();
    m_rows.reserve(spells.size());
    const FProperty* icon_field = spells.empty() ? nullptr : property(spells.front()->GetClassPrivate(), L"SpellIcon");
    auto slot_class = static_cast<UStruct*>(game_class(spell_slot_class_path));
    const auto spell_offset = property(slot_class, L"SpellData")->GetOffset_ForInternal();
    auto unlocked_field = static_cast<FBoolProperty*>(property(slot_class, L"bUnlocked"));
    const auto item_offset = property(slot_class, L"ItemImage")->GetOffset_ForInternal();
    const auto lock_offset = property(slot_class, L"LockImage")->GetOffset_ForInternal();
    for (auto spell : spells)
    {
        auto cell = reinterpret_cast<unsigned char*>(game_widget(slot_class, m_owner));
        Typed<UObject*>{ cell + spell_offset }.write(spell);
        unlocked_field->SetPropertyValue(cell + unlocked_field->GetOffset_ForInternal(), true);
        Call brush(Typed<UObject*>{ cell + item_offset }.read(), L"SetBrushFromSoftTexture");
        auto destination = brush[L"SoftTexture"];
        destination.field->CopyCompleteValue(destination.data, reinterpret_cast<unsigned char*>(spell) + icon_field->GetOffset_ForInternal());
        brush.run();
        set(Typed<UObject*>{ cell + lock_offset }.read(), L"SetVisibility", L"InVisibility", L"Collapsed");
        attach(grid, L"AddChildToWrapBox", reinterpret_cast<UObject*>(cell));
        m_rows.push_back({ spell, Retained<bool>(reinterpret_cast<UObject*>(cell), L"IsHovered", L"ReturnValue") });
    }
    auto panel_slot = attach(m_canvas, L"AddChildToCanvas", size);
    set(panel_slot, L"SetAnchors", L"InAnchors", L"(Minimum=(X=0.5,Y=1),Maximum=(X=0.5,Y=1))");
    set(panel_slot, L"SetAlignment", L"InAlignment", L"(X=0.5,Y=1)");
    set(panel_slot, L"SetPosition", L"InPosition", L"(X=0,Y=-230)");
    set(panel_slot, L"SetAutoSize", L"InbAutoSize", L"True");
    m_picker_visibility.emplace(size, L"SetVisibility", L"InVisibility");
    m_picker_visibility->set(L"Visible");
}

void Bar::close_picker()
{
    m_picker_shown = false;
    m_picker_visibility->set(L"Collapsed");
}

UObject* Bar::picked()
{
    for (auto& row : m_rows)
        if (row.hovered.get()) return row.spell;
    return nullptr;
}

void Bar::show(bool on, bool for_setup)
{
    m_visibility.set(visibility_states[on][for_setup]);
    if (on && !m_visible) Log::write("Spell bar visible.");
    if (!on && m_visible) Log::write("Spell bar hidden.");
    m_visible = on;
    m_hit_testable = on && for_setup;
}

void Bar::release()
{
    if (!m_root_reference.Get()) return;
    m_detach.run();
    m_root->ClearRootSet();
    if (m_root->IsRootSet())
    {
        Log::write("Spell bar root reference was not released.");
        return;
    }
    Log::write("Spell bar released.");
}

void SpellBar::stop()
{
    m_setup = Setup{};
    m_cooldowns = Cooldowns{};
    m_display_dirty = false;
    m_cast_window = 0;
    if (!m_bar) return;
    std::optional<Bar> bar = std::move(m_bar);
    m_bar.reset();
    bar->release();
}

void SpellBar::configure(const Settings::SpellBar& incoming)
{
    if (m_config == incoming) return;
    stop();
    m_config = incoming;
    if (!incoming.enabled) return;
    for (int i = 0; i < 3; ++i)
        if (!m_cast_hooks[i]) m_cast_hooks[i].emplace(cast_events[i].function, [this, window = cast_events[i].window](UObject*) { m_cast_window = std::max(m_cast_window, window); });
}

bool SpellBar::wants_step(const Frame& frame) const
{
    return m_config.enabled && !m_config.slots.empty() && frame.controller && (!m_bar || m_display_dirty);
}

void SpellBar::step(const Frame& frame)
{
    if (!m_bar)
    {
        Widgets::resolve();
        m_bar.emplace(m_config, frame.outer, frame.controller, *frame.player, ++m_generation);
        m_cooldowns.watch(frame.now, 1);
    }
    const bool hit_testable = frame.shown && frame.cursor;
    if (frame.shown != m_bar->visible() || hit_testable != m_bar->hit_testable()) m_bar->show(frame.shown, frame.cursor);
    m_setup.frame(*m_bar, m_config, frame);
    m_cooldowns.update(*m_bar, m_config, frame);
    m_display_dirty = m_setup.active();
}
}
