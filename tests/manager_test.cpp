#include "fake/Fixture.hpp"
#include <chrono>
#include <cstring>
#include <fstream>

namespace
{
struct Manager : Fixture {};

TEST_F(Manager, builds_the_bar_once_and_stays_idle)
{
    auto since = Fake::mark();
    tick();
    ASSERT_FALSE(Fake::rooted());
    configure();
    idle(10);
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::game.adds == 1);
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    Fake::collect();
    ASSERT_TRUE(Fake::rooted() && Fake::all_alive_since(since));
    ASSERT_TRUE(Fake::records.at(Fake::rooted()).outer == Fake::game.game_instance);
    ASSERT_TRUE(!Fake::records.at(Fake::game.controller).marked);
    for (auto tooltip : Fake::game.tooltips) ASSERT_TRUE(Fake::records.at(tooltip).outer == Fake::game.game_instance);
    int labels = 0;
    for (const auto& [pointer, record] : Fake::records)
        if (record.display_text.size() == 2 && record.display_text[0] == L'F') ++labels;
    ASSERT_TRUE(labels == 4 && Fake::game.style_textures == 0);
    tick();
    const auto widgets = Fake::game.constructions;
    idle(100000);
    ASSERT_TRUE(Fake::game.constructions == widgets && Fake::game.scans == 1 && Fake::game.log.empty());
}

TEST_F(Manager, registers_one_key_event_per_bound_slot_and_replaces_them_on_reconfigure)
{
    bind(2, L"OtherSpell", L"Ctrl+Q", static_cast<RC::Input::Key>(0x51), { RC::Input::CONTROL });
    settings.spell_bar.slots[3] = {};
    start();
    auto& events = Fake::game.key_set.key_data;
    ASSERT_EQ(events.size(), 6u);
    ASSERT_TRUE(events.at(RC::Input::LEFT_MOUSE_BUTTON).size() == 1 && events.at(RC::Input::RIGHT_MOUSE_BUTTON).size() == 1 && events.at(RC::Input::MIDDLE_MOUSE_BUTTON).size() == 1);
    ASSERT_TRUE(events.at(static_cast<RC::Input::Key>(0x74)).at(0).required_modifier_keys == 0);
    ASSERT_TRUE(events.at(static_cast<RC::Input::Key>(0x51)).at(0).required_modifier_keys == 1u << RC::Input::CONTROL);
    settings.spell_bar.enabled = false;
    configure();
    ASSERT_TRUE(events.empty());
    settings.spell_bar.enabled = true;
    configure();
    ASSERT_EQ(events.size(), 6u);
    for (const auto& [key, data] : events) ASSERT_EQ(data.size(), 1u);
}

TEST_F(Manager, keeps_the_bar_on_an_unchanged_configuration)
{
    start();
    const auto root = Fake::rooted();
    configure();
    tick();
    ASSERT_TRUE(Fake::rooted() == root && Fake::game.adds == 1 && Fake::game.log.empty());
    idle(10);
}

TEST_F(Manager, tears_down_when_the_controller_dies_and_rebuilds_on_the_next_restart)
{
    start();
    ASSERT_TRUE(Fake::records.at(Fake::game.perk).roots == 1 && Fake::records.at(Fake::game.other_perk).roots == 1 && Fake::records.at(Fake::game.locked_perk).roots == 1);
    end_world();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.removes == 1 && Fake::game.parameter_allocations == 0);
    ASSERT_TRUE(Fake::logged("Spell bar released.") && Fake::game.log.empty());
    ASSERT_TRUE(Fake::records.at(Fake::game.perk).roots == 0 && Fake::records.at(Fake::game.spell).roots == 0);
    Fake::collect();
    for (auto& [pointer, record] : Fake::records)
    {
        if (record.collectable) { ASSERT_FALSE(record.alive); }
    }
    Fake::time += 5000;
    idle(100);
    ASSERT_TRUE(!Fake::rooted() && Fake::game.log.empty());
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::game.adds == 2);
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    tick();
    idle(10);
}

TEST_F(Manager, releases_without_touching_a_bar_the_engine_destroyed)
{
    start();
    auto root = Fake::rooted();
    Fake::records.at(root).alive = false;
    Fake::records.at(root).roots = 0;
    end_world();
    ASSERT_FALSE(Fake::rooted());
    ASSERT_TRUE(Fake::game.parameter_allocations == 0 && Fake::game.removes == 0 && Fake::game.log.empty());
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::rooted() != root);
}

TEST_F(Manager, ignores_a_client_restart_before_the_player_is_ready)
{
    Fake::game.ready = false;
    configure();
    restart();
    tick();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.log.empty());
    Fake::time += 60000;
    idle(10000);
    Fake::game.ready = true;
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::game.adds == 1 && Fake::game.scans == 1);
    tick();
    idle(10);
}

TEST_F(Manager, ignores_a_client_restart_of_a_controller_without_is_player_ready)
{
    configure();
    Fake::Object menu(L"MenuController");
    auto ready = Fake::game.functions.extract(L"IsPlayerReady");
    Fake::call(menu.pointer, L"ClientRestart");
    tick();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.log.empty());
    Fake::game.functions.insert(std::move(ready));
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
}

TEST_F(Manager, keeps_the_tick_when_a_cast_hook_is_absent_and_recovers_on_the_next_file)
{
    auto hook = Fake::game.paths.extract(L"/Script/Dominion.PlayerMagicComponent:Multicast_SendPayloadForSpellCasting");
    std::ofstream(directory / "mods.ini") << "[SpellBar]\nSlot1 = F5,\n";
    restart();
    Fake::time += 3000;
    tick();
    ASSERT_TRUE(!Fake::rooted() && Fake::logged("Spell bar stopped: Required function is absent: /Script/Dominion.PlayerMagicComponent:Multicast_SendPayloadForSpellCasting") && Fake::game.log.empty());
    Fake::time += 3000;
    idle(10);
    Fake::game.paths.insert(std::move(hook));
    std::ofstream(directory / "mods.ini") << "[SpellBar]\nSlot1 = F6,\n";
    std::filesystem::last_write_time(directory / "mods.ini", std::filesystem::last_write_time(directory / "mods.ini") + std::chrono::seconds(2));
    Fake::time += 3000;
    tick();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::logged("Configuration accepted.") && Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
}

TEST_F(Manager, builds_the_catalog_from_the_loaded_skills_and_marks_a_slot_without_perks)
{
    bind(0, L"TestSpell", L"F5", static_cast<RC::Input::Key>(0x74));
    Fake::game.perk_count = 0;
    start();
    ASSERT_TRUE(widget(L"SpellBarKey0").display_text == L"F5 ?" && Fake::game.scans == 1);
    Fake::game.perk_count = 3;
    restart();
    tick();
    ASSERT_TRUE(widget(L"SpellBarKey0").display_text == L"F5" && Fake::records.at(Fake::game.spell).roots == 1 && Fake::game.scans == 2);
}

TEST_F(Manager, rebinds_on_a_client_restart_while_bound)
{
    start();
    auto root = Fake::rooted();
    restart();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.removes == 1);
    ASSERT_TRUE(Fake::logged("Spell bar released.") && Fake::game.log.empty());
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::rooted() != root && Fake::game.adds == 2);
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
}

TEST_F(Manager, stops_the_spell_bar_on_a_construction_fault_until_reconfigured)
{
    start();
    settings.spell_bar.scale = 1.1f;
    configure();
    Fake::game.fail_after = 2;
    tick();
    ASSERT_TRUE(!Fake::rooted());
    ASSERT_TRUE(Fake::logged("Spell bar stopped: Widget construction failed") && Fake::game.log.empty());
    const auto widgets = Fake::game.constructions;
    for (int i = 0; i < 10000; ++i) tick();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.constructions == widgets && Fake::game.log.empty());
    Fake::game.fail_after = -1;
    configure();
    tick();
    ASSERT_TRUE(Fake::rooted());
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
}

TEST_F(Manager, builds_slots_with_icons_and_marks_missing_spells)
{
    Fake::asset(L"/Game/Art/UI/Common/T_Common_ItemSlotsBackground.T_Common_ItemSlotsBackground");
    Fake::asset(L"/Game/UI/Fonts/Poppins-Regular_Font.Poppins-Regular_Font");
    bind(0, L"TestSpell", L"F5", static_cast<RC::Input::Key>(0x74));
    settings.spell_bar.slots[1].spell = L"USD_Missing";
    const auto since = Fake::mark();
    start();
    Fake::collect();
    ASSERT_TRUE(Fake::all_alive_since(since));
    ASSERT_TRUE(Fake::game.last_width == 52 * settings.spell_bar.scale && Fake::game.last_height == 52 * settings.spell_bar.scale);
    ASSERT_TRUE(Fake::game.last_icon_x == 40.0 * settings.spell_bar.scale && Fake::game.last_icon_y == 40.0 * settings.spell_bar.scale);
    ASSERT_TRUE(Fake::game.style_textures == 4);
    bool missing_label = false;
    for (const auto& [pointer, record] : Fake::records)
        if (record.display_text == L"F6 ?") missing_label = true;
    ASSERT_TRUE(missing_label);
    ASSERT_TRUE(widget(L"SpellBarShade0").visibility == L"Collapsed");
    ASSERT_TRUE(widget(L"SpellBarSeconds0").visibility == L"Collapsed");
    ASSERT_TRUE(widget(L"SpellBarShade1").visibility == L"Collapsed");
    idle(10000);
}

TEST_F(Manager, stays_idle_while_disabled)
{
    start();
    settings.spell_bar.enabled = 0;
    configure();
    tick();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.log.empty());
    Fake::time += 5000;
    idle(1000);
    settings.spell_bar.enabled = 1;
    configure();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::game.adds == 2);
}

TEST_F(Manager, stops_the_spell_bar_when_the_root_is_not_retained)
{
    Fake::game.rooting_works = false;
    configure();
    restart();
    tick();
    ASSERT_FALSE(Fake::rooted());
    ASSERT_TRUE(Fake::logged("Spell bar stopped: Widget root reference was not retained") && Fake::game.log.empty());
    const auto constructions = Fake::game.constructions;
    for (int i = 0; i < 1000; ++i) tick();
    ASSERT_TRUE(Fake::game.constructions == constructions && Fake::game.log.empty());
    Fake::game.rooting_works = true;
    settings.spell_bar.scale = 1.3f;
    configure();
    tick();
    ASSERT_TRUE(Fake::rooted());
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
}

TEST_F(Manager, releases_everything_on_unload_and_reports_a_root_that_stays_rooted)
{
    start();
    auto root = Fake::rooted();
    Fake::game.clearing_works = false;
    mod.reset();
    ASSERT_TRUE(Fake::rooted() == root && Fake::game.removes == 1 && Fake::game.parameter_allocations == 0);
    ASSERT_TRUE(Fake::logged("Spell bar root reference was not released.") && Fake::game.log.empty());
    Fake::game.clearing_works = true;
    Fake::records.at(root).roots = 0;
    mod.emplace(directory);
    mod->on_unreal_init();
    Fake::game.log.clear();
    configure();
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::rooted() != root);
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    mod.reset();
    ASSERT_TRUE(!Fake::rooted() && Fake::game.parameter_allocations == 0);
    ASSERT_TRUE(Fake::logged("Spell bar released.") && Fake::game.log.empty());
    Fake::collect();
    for (auto& [pointer, record] : Fake::records)
    {
        ASSERT_TRUE(record.roots == 0);
        if (!record.collectable) continue;
        ASSERT_FALSE(record.alive);
        if (record.name == L"Widget") { ASSERT_TRUE(record.object_flags & 0x40); }
    }
    mod.emplace(directory);
}

TEST_F(Manager, unregisters_everything_on_unload_and_survives_a_reload)
{
    start();
    ASSERT_TRUE(Fake::game.callbacks.size() == 1 && Fake::engine_tick && Fake::game.key_set.key_data.size() == 7);
    mod.reset();
    ASSERT_TRUE(Fake::game.callbacks.empty() && !Fake::engine_tick && Fake::game.key_set.key_data.empty());
    ASSERT_TRUE(!Fake::rooted() && Fake::game.parameter_allocations == 0);
    ASSERT_TRUE(Fake::logged("Spell bar released.") && Fake::game.log.empty());
    for (auto& [hook, custom] : Fake::records.at(Fake::game.functions.at(L"ClientRestart")).post_hooks) ASSERT_FALSE(hook);
    mod.emplace(directory);
    mod->on_unreal_init();
    ASSERT_TRUE(Fake::game.callbacks.size() == 1 && Fake::logged("Spell bar mod started.") && Fake::game.log.empty());
    configure();
    restart();
    tick();
    ASSERT_TRUE(Fake::rooted() && Fake::game.adds == 2 && Fake::game.key_set.key_data.size() == 7);
    end_world();
    ASSERT_FALSE(Fake::rooted());
}
}
