#include "fake/Fixture.hpp"
#include <cstring>
#include <fstream>

namespace
{
constexpr auto F5 = static_cast<RC::Input::Key>(0x74);
constexpr auto F6 = static_cast<RC::Input::Key>(0x75);
constexpr auto Q = static_cast<RC::Input::Key>(0x51);

struct Actions : Fixture
{
    void SetUp() override
    {
        bind(0, L"TestSpell", L"F5", F5);
        start();
    }
};

struct Bindings : Fixture {};

TEST_F(Actions, blocks_presses_outside_gameplay)
{
    auto gameplay = gameplay_mode();
    input_mode(nullptr);
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && root_visibility() == L"Hidden");
    input_mode(gameplay);
    tick();
    ASSERT_EQ(root_visibility(), L"HitTestInvisible");
    input_mode(Fake::game.casting_mode);
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && root_visibility() == L"HitTestInvisible");
    input_mode(gameplay);
    member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", true);
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && root_visibility() == L"Visible");
    member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", false);
    tick();
    ASSERT_TRUE(Fake::game.selections == 0 && Fake::game.page_changes == 0);
    ASSERT_TRUE(Fake::logged("Spell bar hidden.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    press(F5);
    ASSERT_EQ(Fake::game.selections, 1);
}

TEST_F(Actions, refuses_while_the_wheel_is_not_created)
{
    Fake::game.wheel_available = false;
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && Fake::logged("not ready") && Fake::game.log.empty());
    Fake::game.wheel_available = true;
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 1 && Fake::game.log.empty());
}

TEST_F(Actions, selects_the_page_and_slot_through_the_wheel)
{
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 1 && Fake::game.page_changes == 1 && Fake::game.selected_page == 2 && Fake::game.log.empty());
    ASSERT_TRUE(member(Fake::game.wheel, L"CachedSectionId").read<uint8_t>(L"ByteProperty") == 3 && Fake::game.current_slot == 3);
    const auto scans = Fake::game.scans;
    press(F5);
    ASSERT_TRUE(Fake::game.scans == scans && Fake::game.selections == 2 && Fake::game.page_changes == 1);
}

TEST_F(Actions, changes_page_when_the_spell_is_assigned_twice_or_not_selected)
{
    press(F5);
    ASSERT_TRUE(Fake::game.page_changes == 1 && Fake::game.selected_page == 2);
    Fake::game.assigned_spells[3] = Fake::game.spell;
    press(F5);
    ASSERT_TRUE(Fake::game.page_changes == 2 && Fake::game.selected_page == 0 && Fake::game.current_slot == 3);
    Fake::game.assigned_spells[3] = nullptr;
    press(F5);
    ASSERT_TRUE(Fake::game.page_changes == 3 && Fake::game.selected_page == 2);
    Fake::game.selection_allowed = false;
    Fake::game.current_slot = 0;
    press(F5);
    ASSERT_TRUE(Fake::game.current_slot == 0 && Fake::game.page_changes == 4);
}

TEST_F(Actions, refuses_a_spell_that_is_not_on_the_wheel)
{
    Fake::game.assigned_spells[27] = nullptr;
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && Fake::logged("not loaded or not assigned") && Fake::game.log.empty());
}

TEST_F(Actions, stops_the_spell_bar_after_a_dead_slice_until_reconfigured)
{
    Fake::records.at(Fake::game.slices[3]).alive = false;
    press(F5);
    ASSERT_TRUE(Fake::logged("Spell bar stopped: Spell wheel is not initialized for selection") && Fake::logged("Spell bar released.") && Fake::game.log.empty());
    ASSERT_FALSE(Fake::rooted());
    Fake::records.at(Fake::game.slices[3]).alive = true;
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 0 && !Fake::rooted() && Fake::game.log.empty());
    configure();
    tick();
    ASSERT_EQ(root_visibility(), L"HitTestInvisible");
    press(F5);
    ASSERT_TRUE(Fake::game.selections == 1);
}

TEST_F(Actions, drops_a_press_recorded_before_a_reload_in_the_same_tick)
{
    std::ofstream(directory / "mods.ini") << "[SpellBar]\nSlot1 = F6, OtherSpell\n";
    Fake::press(F5);
    Fake::time += 3000;
    tick();
    ASSERT_TRUE(Fake::logged("Configuration accepted.") && Fake::logged("Spell bar released.") && Fake::game.selections == 0);
    tick();
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    press(F5);
    ASSERT_EQ(Fake::game.selections, 0);
    press(F6);
    ASSERT_TRUE(Fake::game.selections == 1 && Fake::game.selected_page == 0 && Fake::game.current_slot == 5);
}

TEST_F(Actions, releases_wheel_calls_when_the_controller_ends_play_in_a_dead_world)
{
    press(F5);
    ASSERT_EQ(Fake::game.selections, 1);
    Fake::destroy_world_functions();
    end_world();
    ASSERT_FALSE(Fake::rooted());
    ASSERT_EQ(Fake::game.parameter_allocations, 0u);
}

TEST_F(Bindings, fires_only_on_the_exact_modifiers)
{
    bind(2, L"OtherSpell", L"Ctrl+Q", Q, { RC::Input::CONTROL });
    start();
    press(Q);
    ASSERT_TRUE(Fake::game.selections == 0);
    press(Q, { RC::Input::CONTROL, RC::Input::SHIFT });
    ASSERT_TRUE(Fake::game.selections == 0);
    press(Q, { RC::Input::CONTROL });
    ASSERT_TRUE(Fake::game.selections == 1 && Fake::game.page_changes == 1 && Fake::game.selected_page == 0 && Fake::game.current_slot == 5);
}
}
