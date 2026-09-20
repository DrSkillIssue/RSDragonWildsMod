#include "Fake.hpp"
#include <cstring>

namespace Fake
{
namespace
{
constexpr const wchar_t* ui_paths[] = {
    L"/Script/UMG.UserWidget", L"/Script/UMG.WidgetTree", L"/Script/UMG.CanvasPanel",
    L"/Script/UMG.InvalidationBox", L"/Script/UMG.HorizontalBox", L"/Script/UMG.SizeBox",
    L"/Script/UMG.Border", L"/Script/UMG.TextBlock", L"/Script/UMG.Image", L"/Script/UMG.VerticalBox", L"/Script/UMG.ScrollBox", L"/Script/UMG.WrapBox"
};
}

UClass* widget_class{};
UFunction* hooked_functions[4]{};

void setup()
{
    if (!widget_class)
    {
        Object widget(L"Widget"); widget_class = reinterpret_cast<UClass*>(widget.pointer);
        Object font(L"SlateFontInfo"); auto font_type = reinterpret_cast<UScriptStruct*>(font.pointer);
        field(font_type,L"Size",L"FloatProperty",0,4);
        field(font_type,L"FontObject",L"ObjectProperty",8,8);
        field(font_type,L"TypefaceFontName",L"NameProperty",16,8);
        field(font_type,L"LetterSpacing",L"IntProperty",24,4);
        Object brush(L"SlateBrush"); auto brush_type = reinterpret_cast<UScriptStruct*>(brush.pointer);
        field(brush_type,L"DrawAs",L"EnumProperty",0,1);
        field(brush_type,L"Margin",L"StructProperty",8,16);
        field(widget_class,L"Background",L"StructProperty",128,176,brush_type);
        field(widget_class,L"WidgetTree",L"ObjectProperty",0,8);
        field(widget_class,L"RootWidget",L"ObjectProperty",8,8);
        field(widget_class,L"Font",L"StructProperty",16,96,font_type);
        field(widget_class,L"TickFrequency",L"EnumProperty",112,1);
        field(widget_class,L"bIsFocusable",L"BoolProperty",113,1);
    }
    records.at(widget_class).alive = true;
    for (auto path : ui_paths) game.paths.emplace(path,reinterpret_cast<UObject*>(widget_class));
    if (!hooked_functions[0])
    {
        hooked_functions[0] = function(L"Server_NotifyPreparingSpell",8,{{L"SpellBeingPrepared",L"ObjectProperty",0,8}});
        hooked_functions[1] = function(L"Multicast_SendPayloadForSpellCasting",2,{{L"SpellNetId",L"StructProperty",0,2}});
        hooked_functions[2] = function(L"ClientRestart",8,{{L"NewPawn",L"ObjectProperty",0,8}});
        hooked_functions[3] = function(L"HandleOnSpellCastingAnimationFinished",0,{});
    }
    for (auto hooked : hooked_functions)
    {
        records.at(hooked).alive = true;
        game.functions.emplace(records.at(hooked).name, hooked);
    }
    game.paths.emplace(L"/Script/Dominion.PlayerUtilityMagicComponent:Server_NotifyPreparingSpell",reinterpret_cast<UObject*>(hooked_functions[0]));
    game.paths.emplace(L"/Script/Dominion.PlayerMagicComponent:Multicast_SendPayloadForSpellCasting",reinterpret_cast<UObject*>(hooked_functions[1]));
    game.paths.emplace(L"/Script/Engine.PlayerController:ClientRestart",reinterpret_cast<UObject*>(hooked_functions[2]));
    game.paths.emplace(L"/Script/Dominion.PlayerMagicComponent:HandleOnSpellCastingAnimationFinished",reinterpret_cast<UObject*>(hooked_functions[3]));
    Object library(L"KismetSystemLibrary"); game.paths.emplace(L"/Script/Engine.Default__KismetSystemLibrary",library.pointer);
    function(L"Conv_ObjectToSoftObjectReference",48,{{L"Object",L"ObjectProperty",0,8},{L"ReturnValue",L"SoftObjectProperty",8,40}});
    function(L"LoadClassAsset_Blocking",48,{{L"AssetClass",L"SoftClassProperty",0,40},{L"ReturnValue",L"ClassProperty",40,8}});
    function(L"LoadAsset_Blocking",48,{{L"Asset",L"SoftObjectProperty",0,40},{L"ReturnValue",L"ObjectProperty",40,8}});
    Object widget_library(L"WidgetBlueprintLibrary"); game.paths.emplace(L"/Script/UMG.Default__WidgetBlueprintLibrary",widget_library.pointer);
    function(L"Create",32,{{L"WorldContextObject",L"ObjectProperty",0,8},{L"WidgetType",L"ClassProperty",8,8},{L"OwningPlayer",L"ObjectProperty",16,8},{L"ReturnValue",L"ObjectProperty",24,8}});
    function(L"SetToolTip",8,{{L"Widget",L"ObjectProperty",0,8}});
    Object tooltip_class(L"WBP_TextOnlyTooltip_C");
    field(reinterpret_cast<UStruct*>(tooltip_class.pointer),L"Title",L"ObjectProperty",0,8);
    field(reinterpret_cast<UStruct*>(tooltip_class.pointer),L"Text",L"ObjectProperty",8,8);
    game.loadable.emplace(L"/Game/UI/Common/WBP_TextOnlyTooltip.WBP_TextOnlyTooltip_C",tooltip_class.pointer);
    Object slot_class(L"WBP_SpellSlot_C");
    field(reinterpret_cast<UStruct*>(slot_class.pointer),L"ItemImage",L"ObjectProperty",0,8);
    field(reinterpret_cast<UStruct*>(slot_class.pointer),L"LockImage",L"ObjectProperty",8,8);
    field(reinterpret_cast<UStruct*>(slot_class.pointer),L"SpellData",L"ObjectProperty",16,8);
    field(reinterpret_cast<UStruct*>(slot_class.pointer),L"bUnlocked",L"BoolProperty",24,1);
    game.loadable.emplace(L"/Game/UI/InGameMenus/TopNavScreens/SpellBook/WBP_SpellSlot.WBP_SpellSlot_C",slot_class.pointer);
    function(L"SetInnerSlotPadding",16,{{L"InPadding",L"StructProperty",0,16}});
    function(L"IsPlayerReady",1,{{L"ReturnValue",L"BoolProperty",0,1}});
    function(L"RemoveFromParent",0,{});
    function(L"SetVisibility",1,{{L"InVisibility",L"EnumProperty",0,1}});
    function(L"SetOwningPlayer",8,{{L"LocalPlayerController",L"ObjectProperty",0,8}});
    for (auto name : {L"AddChildToCanvas",L"SetContent",L"AddChildToHorizontalBox",L"AddChildToVerticalBox",L"AddChild",L"AddChildToWrapBox"})
        function(name,16,{{L"Content",L"ObjectProperty",0,8},{L"ReturnValue",L"ObjectProperty",8,8}});
    function(L"SetVerticalAlignment",1,{{L"InVerticalAlignment",L"EnumProperty",0,1}});
    function(L"SetSize",8,{{L"InSize",L"StructProperty",0,8}});
    function(L"SetScrollbarThickness",16,{{L"NewScrollbarThickness",L"StructProperty",0,16}});
    function(L"SetHorizontalAlignment",1,{{L"InHorizontalAlignment",L"EnumProperty",0,1}});
    function(L"SetAnchors",32,{{L"InAnchors",L"StructProperty",0,32}});
    function(L"SetAlignment",16,{{L"InAlignment",L"StructProperty",0,16}});
    function(L"SetPosition",16,{{L"InPosition",L"StructProperty",0,16}});
    function(L"SetAutoSize",1,{{L"InbAutoSize",L"BoolProperty",0,1}});
    function(L"SetWidthOverride",4,{{L"InWidthOverride",L"FloatProperty",0,4}});
    function(L"SetHeightOverride",4,{{L"InHeightOverride",L"FloatProperty",0,4}});
    function(L"SetBrushColor",16,{{L"InBrushColor",L"StructProperty",0,16}});
    function(L"SetBrush",176,{{L"InBrush",L"StructProperty",0,176}});
    function(L"SetBrushFromTexture",8,{{L"Texture",L"ObjectProperty",0,8}});
    function(L"SetPadding",16,{{L"InPadding",L"StructProperty",0,16}});
    function(L"SetText",16,{{L"InText",L"TextProperty",0,16}});
    function(L"SetColorAndOpacity",20,{{L"InColorAndOpacity",L"StructProperty",0,20}});
    function(L"SetShadowColorAndOpacity",16,{{L"InShadowColorAndOpacity",L"StructProperty",0,16}});
    function(L"SetShadowOffset",16,{{L"InShadowOffset",L"StructProperty",0,16}});
    function(L"SetZOrder",4,{{L"InZOrder",L"IntProperty",0,4}});
    function(L"AddToViewport",4,{{L"ZOrder",L"IntProperty",0,4}});
    function(L"SetDesiredSizeOverride",16,{{L"DesiredSize",L"StructProperty",0,16}});
    function(L"SetRenderScale",16,{{L"Scale",L"StructProperty",0,16}});
    function(L"SetRenderTransformPivot",16,{{L"Pivot",L"StructProperty",0,16}});
    function(L"IsHovered",1,{{L"ReturnValue",L"BoolProperty",0,1}});
    Object vector2(L"Vector2D");
    auto vector_type=reinterpret_cast<UScriptStruct*>(vector2.pointer);
    field(vector_type,L"X",L"DoubleProperty",0,8);
    field(vector_type,L"Y",L"DoubleProperty",8,8);
    records.at(game.functions.at(L"SetDesiredSizeOverride")->GetChildProperties()).structure=vector_type;
    records.at(game.functions.at(L"SetRenderScale")->GetChildProperties()).structure=vector_type;
    records.at(game.functions.at(L"SetScrollbarThickness")->GetChildProperties()).structure=vector_type;
    records.at(game.functions.at(L"SetInnerSlotPadding")->GetChildProperties()).structure=vector_type;
    function(L"SetBrushFromSoftTexture",41,{{L"SoftTexture",L"SoftObjectProperty",0,40},{L"bMatchSize",L"BoolProperty",40,1}});
    Object spell_type(L"UtilitySpellData");
    auto st=reinterpret_cast<UStruct*>(spell_type.pointer);
    field(st,L"SpellIcon",L"SoftObjectProperty",0,40);
    field(st,L"CooldownDuration",L"FloatProperty",40,4);
    field(st,L"SpellDisplayName",L"TextProperty",48,16);
    field(st,L"bNeedsUnlocking",L"BoolProperty",64,1);
    Object modifier_struct(L"UtilitySpellCooldownModifierPerk"); auto modifier_type=reinterpret_cast<UScriptStruct*>(modifier_struct.pointer);
    field(modifier_type,L"CooldownPerk",L"ObjectProperty",0,8);
    field(modifier_type,L"ModifiedCooldown",L"FloatProperty",8,4);
    field(st,L"CooldownModifierPerk",L"StructProperty",72,16,modifier_type);
    static const wchar_t* spell_name=L"Test Spell", *other_name=L"Other Spell";
    Object spell_asset(L"TestSpell",reinterpret_cast<UClass*>(spell_type.pointer));
    game.spell=spell_asset.pointer;
    member(game.spell,L"CooldownDuration").write<float>(L"FloatProperty",30);
    std::memcpy(reinterpret_cast<unsigned char*>(game.spell)+48,&spell_name,8);
    Object other_asset(L"OtherSpell",reinterpret_cast<UClass*>(spell_type.pointer));
    game.other=other_asset.pointer;
    member(game.other,L"CooldownDuration").write<float>(L"FloatProperty",12);
    std::memcpy(reinterpret_cast<unsigned char*>(game.other)+48,&other_name,8);
    member(game.spell,L"bNeedsUnlocking").write(L"BoolProperty",true);
    Object locked_asset(L"LockedSpell",reinterpret_cast<UClass*>(spell_type.pointer));
    game.locked_spell=locked_asset.pointer;
    member(game.locked_spell,L"bNeedsUnlocking").write(L"BoolProperty",true);
    Object module_type(L"SkillPerkModule_Spell"), recipe_module_type(L"SkillPerkModule_Recipes"), perk_type(L"SkillPerkData"), perk_element(L"PerkModuleElement");
    records.at(perk_element.pointer).kind=L"ObjectProperty"; records.at(perk_element.pointer).size=8;
    field(reinterpret_cast<UStruct*>(module_type.pointer),L"SpellData",L"ObjectProperty",0,8);
    field(reinterpret_cast<UStruct*>(perk_type.pointer),L"PerkModules",L"ArrayProperty",0,16,nullptr,reinterpret_cast<FProperty*>(perk_element.pointer));
    Object module_a(L"ModuleA",reinterpret_cast<UClass*>(module_type.pointer)), module_b(L"ModuleB",reinterpret_cast<UClass*>(module_type.pointer)), module_c(L"ModuleC",reinterpret_cast<UClass*>(module_type.pointer)), recipes(L"Recipes",reinterpret_cast<UClass*>(recipe_module_type.pointer));
    member(module_a.pointer,L"SpellData").write(L"ObjectProperty",game.spell);
    member(module_b.pointer,L"SpellData").write(L"ObjectProperty",game.other);
    member(module_c.pointer,L"SpellData").write(L"ObjectProperty",game.locked_spell);
    Object perk_a(L"PerkA",reinterpret_cast<UClass*>(perk_type.pointer)), perk_b(L"PerkB",reinterpret_cast<UClass*>(perk_type.pointer)), perk_c(L"PerkC",reinterpret_cast<UClass*>(perk_type.pointer));
    static UObject* modules_a[2]; modules_a[0]=recipes.pointer; modules_a[1]=module_a.pointer;
    static UObject* modules_b[1]; modules_b[0]=module_b.pointer;
    static UObject* modules_c[1]; modules_c[0]=module_c.pointer;
    member(perk_a.pointer,L"PerkModules").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(modules_a),2,2});
    member(perk_b.pointer,L"PerkModules").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(modules_b),1,1});
    member(perk_c.pointer,L"PerkModules").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(modules_c),1,1});
    game.locked_perk=perk_c.pointer;
    member(game.spell,L"CooldownModifierPerk").member(L"CooldownPerk").write(L"ObjectProperty",perk_a.pointer);
    member(game.spell,L"CooldownModifierPerk").member(L"ModifiedCooldown").write<float>(L"FloatProperty",25);
    member(game.other,L"CooldownModifierPerk").member(L"CooldownPerk").write(L"ObjectProperty",perk_c.pointer);
    member(game.other,L"CooldownModifierPerk").member(L"ModifiedCooldown").write<float>(L"FloatProperty",5);
    UObject* perks[3]={perk_a.pointer,perk_b.pointer,perk_c.pointer};
    for (int i=0;i<3;++i) std::memcpy(game.perk_soft+i*40,&perks[i],8);
    Object skill(L"MagicSkill"), skill_default(L"Default__SkillData"); game.skill=skill.pointer; game.skill_default=skill_default.pointer;
    records.at(game.skill_default).object_flags=16;
    Object perk_component_type(L"SkillPerkComponent");
    Object perk_component(L"PerkComponent",reinterpret_cast<UClass*>(perk_component_type.pointer)); game.perk_component=perk_component.pointer;
    Object soft_element(L"SoftPerkElement"); records.at(soft_element.pointer).kind=L"SoftObjectProperty"; records.at(soft_element.pointer).size=40;
    function(L"GetPerksForSkill",24,{{L"InSkillData",L"ObjectProperty",0,8},{L"ReturnValue",L"ArrayProperty",8,16}});
    records.at(records.at(game.functions.at(L"GetPerksForSkill")).children).inner=reinterpret_cast<FProperty*>(soft_element.pointer);
    function(L"IsPerkUnlocked",9,{{L"InSkillPerkData",L"ObjectProperty",0,8},{L"ReturnValue",L"BoolProperty",8,1}});

    Object world_type(L"World"), instance_type(L"GameInstance"), local_type(L"LocalPlayer"), player_type(L"PlayerController"), element(L"ObjectElement");
    Object viewport_type(L"GameViewportClient");
    field(reinterpret_cast<UStruct*>(viewport_type.pointer),L"World",L"ObjectProperty",120,8);
    records.at(element.pointer).kind = L"ObjectProperty"; records.at(element.pointer).size = 8;
    field(reinterpret_cast<UStruct*>(world_type.pointer),L"OwningGameInstance",L"ObjectProperty",0,8);
    field(reinterpret_cast<UStruct*>(instance_type.pointer),L"LocalPlayers",L"ArrayProperty",0,16,nullptr,reinterpret_cast<FProperty*>(element.pointer));
    field(reinterpret_cast<UStruct*>(local_type.pointer),L"PlayerController",L"ObjectProperty",0,8);
    auto pc_type = reinterpret_cast<UClass*>(player_type.pointer);
    field(pc_type,L"Pawn",L"ObjectProperty",0,8);
    field(pc_type,L"CurrentInputMode",L"ObjectProperty",8,8);
    field(pc_type,L"GameplayInputMode",L"ObjectProperty",16,8);
    field(pc_type,L"GameplayLockOnTargetingInputMode",L"ObjectProperty",24,8);
    field(pc_type,L"bShowMouseCursor",L"BoolProperty",48,1);
    field(pc_type,L"RadialMenuInputMode",L"ObjectProperty",56,8);
    field(pc_type,L"SpellcastingModeInputMode",L"ObjectProperty",64,8);
    field(pc_type,L"SpellPlacementModeInputMode",L"ObjectProperty",72,8);
    field(pc_type,L"SpellPlacementModeAdvancedMovementInputMode",L"ObjectProperty",80,8);
    field(pc_type,L"SkillPerkComponent",L"ObjectProperty",88,8);
    Object world(L"WorldInstance",reinterpret_cast<UClass*>(world_type.pointer));
    Object instance(L"GameInstance",reinterpret_cast<UClass*>(instance_type.pointer));
    Object local(L"LocalPlayer",reinterpret_cast<UClass*>(local_type.pointer));
    Object pawn_type(L"PlayerCharacter"), magic_type(L"PlayerUtilityMagicComponent"), float_element(L"FloatElement");
    records.at(float_element.pointer).kind=L"FloatProperty"; records.at(float_element.pointer).size=4;
    field(reinterpret_cast<UStruct*>(pawn_type.pointer),L"PlayerUtilityMagicComponent",L"ObjectProperty",0,8);
    field(reinterpret_cast<UStruct*>(magic_type.pointer),L"LastCastTimeBySpellData",L"MapProperty",0,80,nullptr,reinterpret_cast<FProperty*>(element.pointer));
    records.at(records.at(magic_type.pointer).children).value_inner=reinterpret_cast<FProperty*>(float_element.pointer);
    Object player(L"Player",pc_type), view(L"Viewport",reinterpret_cast<UClass*>(viewport_type.pointer));
    Object pawn(L"Pawn",reinterpret_cast<UClass*>(pawn_type.pointer)), magic(L"Magic",reinterpret_cast<UClass*>(magic_type.pointer)), mode(L"Gameplay"), radial(L"RadialMode");
    Object casting(L"CastingMode"), placement(L"PlacementMode"), advanced(L"AdvancedPlacementMode");
    game.radial_mode=radial.pointer; game.casting_mode=casting.pointer;
    member(pawn.pointer,L"PlayerUtilityMagicComponent").write(L"ObjectProperty",magic.pointer);
    game.magic=magic.pointer;
    game.cast_map=reinterpret_cast<unsigned char*>(magic.pointer);
    { unsigned char* data=game.cast_pairs; int zero=0, capacity=4; std::memcpy(game.cast_map,&data,8); std::memcpy(game.cast_map+8,&zero,4); std::memcpy(game.cast_map+12,&capacity,4); }
    game.controller = player.pointer; game.viewport = view.pointer;
    game.game_instance=instance.pointer;
    records.at(game.controller).outer=world.pointer;
    records.at(game.controller).world = reinterpret_cast<UWorld*>(world.pointer);
    member(game.viewport,L"World").write(L"ObjectProperty",world.pointer);
    member(world.pointer,L"OwningGameInstance").write(L"ObjectProperty",instance.pointer);
    static UObject* locals[1]; locals[0] = local.pointer;
    member(instance.pointer,L"LocalPlayers").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(locals),1,1});
    member(local.pointer,L"PlayerController").write(L"ObjectProperty",game.controller);
    member(game.controller,L"Pawn").write(L"ObjectProperty",pawn.pointer);
    member(game.controller,L"CurrentInputMode").write(L"ObjectProperty",mode.pointer);
    member(game.controller,L"GameplayInputMode").write(L"ObjectProperty",mode.pointer);
    member(game.controller,L"RadialMenuInputMode").write(L"ObjectProperty",radial.pointer);
    member(game.controller,L"SpellcastingModeInputMode").write(L"ObjectProperty",casting.pointer);
    member(game.controller,L"SpellPlacementModeInputMode").write(L"ObjectProperty",placement.pointer);
    member(game.controller,L"SpellPlacementModeAdvancedMovementInputMode").write(L"ObjectProperty",advanced.pointer);
    member(game.controller,L"SkillPerkComponent").write(L"ObjectProperty",perk_component.pointer);

    field(pc_type,L"SpellcastingComponent",L"ObjectProperty",32,8);
    field(pc_type,L"PlayerInput",L"ObjectProperty",40,8);
    Object component_type(L"SpellcastingComponent"), input_type(L"EnhancedPlayerInput"), radial_type(L"WBP_SurvivalSorcery_RadialSelector_C");
    auto ct=reinterpret_cast<UClass*>(component_type.pointer);
    field(ct,L"NumSpellRadials",L"ByteProperty",0,1);
    field(ct,L"NumSpellSlotsPerRadial",L"ByteProperty",1,1);
    field(ct,L"SelectedSpells",L"ArrayProperty",8,16,nullptr,reinterpret_cast<FProperty*>(element.pointer));
    Object spells_instance(L"Spells",ct); game.component=spells_instance.pointer;
    member(game.component,L"NumSpellRadials").write<uint8_t>(L"ByteProperty",4);
    member(game.component,L"NumSpellSlotsPerRadial").write<uint8_t>(L"ByteProperty",12);
    game.assigned_spells[27]=game.spell;
    game.assigned_spells[5]=game.other;
    member(game.component,L"SelectedSpells").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(game.assigned_spells),48,48});
    member(game.controller,L"SpellcastingComponent").write(L"ObjectProperty",game.component);
    Object mapping_type(L"EnhancedActionKeyMapping"), key_type(L"Key"), mapping_element(L"MappingElement");
    auto mt=reinterpret_cast<UScriptStruct*>(mapping_type.pointer), kt=reinterpret_cast<UScriptStruct*>(key_type.pointer);
    field(mt,L"Key",L"StructProperty",0,8,kt);
    field(kt,L"KeyName",L"NameProperty",0,sizeof(FName));
    auto& mapping=records.at(mapping_element.pointer);mapping.kind=L"StructProperty";mapping.size=8;mapping.structure=mt;
    auto it=reinterpret_cast<UClass*>(input_type.pointer);
    field(it,L"EnhancedActionMappings",L"ArrayProperty",0,16,nullptr,reinterpret_cast<FProperty*>(mapping_element.pointer));
    Object inputs(L"Input",it);game.player_input=inputs.pointer;
    member(game.player_input,L"EnhancedActionMappings").write(L"ArrayProperty",Array{game.mapping_data,0,1});
    member(game.controller,L"PlayerInput").write(L"ObjectProperty",game.player_input);
    auto rt=reinterpret_cast<UClass*>(radial_type.pointer);
    field(rt,L"bIsSpellbookInstance",L"BoolProperty",0,1);
    field(rt,L"CachedSectionId",L"ByteProperty",1,1);
    field(rt,L"Slices",L"ArrayProperty",8,16,nullptr,reinterpret_cast<FProperty*>(element.pointer));
    for(auto& slice:game.slices) { Object item(L"Slice"); slice=item.pointer; }
    Object stencil(L"WheelTemplate",rt), book(L"WheelBook",rt), radial_widget(L"Wheel",rt);
    game.wheel_template=stencil.pointer; game.spellbook_wheel=book.pointer; game.wheel=radial_widget.pointer;
    records.at(game.wheel_template).object_flags=16;
    for(auto w:{game.wheel_template,game.spellbook_wheel,game.wheel})
    {
        records.at(w).world=records.at(game.controller).world;
        member(w,L"Slices").write(L"ArrayProperty",Array{reinterpret_cast<unsigned char*>(game.slices),12,12});
    }
    member(game.spellbook_wheel,L"bIsSpellbookInstance").write(L"BoolProperty",true);
    function(L"GetOwningPlayer",8,{{L"ReturnValue",L"ObjectProperty",0,8}});
    function(L"OnSpellRadialChanged",2,{{L"SelectedIndex",L"UInt16Property",0,2}});
    function(L"SelectSlice",0,{});
    function(L"GetCurrentlySelectedSpellData",8,{{L"ReturnValue",L"ObjectProperty",0,8}});

    Object statics(L"GameplayStatics"); game.paths.emplace(L"/Script/Engine.Default__GameplayStatics",statics.pointer);
    function(L"GetTimeSeconds",16,{{L"WorldContextObject",L"ObjectProperty",0,8},{L"ReturnValue",L"DoubleProperty",8,8}});
    Object text_library(L"KismetTextLibrary"); game.paths.emplace(L"/Script/Engine.Default__KismetTextLibrary",text_library.pointer);
    function(L"Conv_TextToString",32,{{L"InText",L"TextProperty",0,16},{L"ReturnValue",L"StrProperty",16,16}});
}
}
