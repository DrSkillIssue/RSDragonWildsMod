#include "fake/Fixture.hpp"
#include <cstring>
#include <fstream>
#include <iterator>

namespace
{
struct Setup : Fixture
{
    UObject* root;
    UObject* frame0;

    void SetUp() override
    {
        std::ofstream(directory / "mods.ini") << "[SpellBar]\nSlot1 = F5, TestSpell\nSlot2 = F6,\nSlot3 = Ctrl+Q, OtherSpell\nSlot4 = F8,\n";
        restart();
        tick();
        root = Fake::rooted();
        ASSERT_TRUE(root);
        frame0 = Fake::game.named.at(L"SpellBarFrame0");
        member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", true);
        tick();
        ASSERT_EQ(Fake::records.at(root).visibility, L"Visible");
        Fake::game.log.clear();
    }

    std::string ini()
    {
        std::ifstream in(directory / "mods.ini");
        return std::string(std::istreambuf_iterator<char>(in), {});
    }

    UObject* tooltip(int slot) { return Fake::game.tooltips.at(Fake::game.tooltips.size() - 4 + slot); }
    const std::wstring& title(int slot) { return Fake::records.at(Fake::object_field(tooltip(slot), 0)).display_text; }
    const std::wstring& text(int slot) { return Fake::records.at(Fake::object_field(tooltip(slot), 8)).display_text; }
};

TEST_F(Setup, builds_the_game_tooltip_for_every_slot)
{
    ASSERT_EQ(Fake::game.tooltips.size(), 4u);
    ASSERT_EQ(Fake::game.class_loads, 1u);
    ASSERT_EQ(title(0), L"TEST SPELL");
    ASSERT_EQ(text(0), L"Cooldown 25 s\nKey: F5\nRight click: Choose spell\nMiddle click: Clear spell");
    ASSERT_EQ(title(2), L"OTHER SPELL");
    ASSERT_EQ(text(2), L"Cooldown 12 s\nKey: Ctrl+Q\nRight click: Choose spell\nMiddle click: Clear spell");
    ASSERT_EQ(title(3), L"EMPTY SLOT");
    ASSERT_EQ(text(3), L"Key: F8\nRight click: Choose spell\nMiddle click: Clear spell");
    const auto& attached = Fake::records.at(frame0).references;
    ASSERT_TRUE(std::find(attached.begin(), attached.end(), Fake::game.tooltips[0]) != attached.end());
}

TEST_F(Setup, makes_no_engine_calls_while_a_menu_is_open_and_idle)
{
    Fake::game.hovered_frame = frame0;
    idle(100);
    ASSERT_TRUE(Fake::game.log.empty());
}

TEST_F(Setup, assigns_and_clears_the_spell)
{
    Fake::game.hovered_frame = frame0;
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    Fake::game.hovered_frame = Fake::game.rows[0];
    press(RC::Input::LEFT_MOUSE_BUTTON);
    ASSERT_TRUE(ini().find("Slot1 = F5, OtherSpell\n") != std::string::npos);
    ASSERT_TRUE(Fake::logged("Configuration saved from the in-game panel.") && Fake::logged("Configuration accepted.") && Fake::logged("Spell bar released.") && Fake::game.log.empty());
    tick();
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    ASSERT_EQ(Fake::game.tooltips.size(), 8u);
    Fake::game.hovered_frame = Fake::game.named.at(L"SpellBarFrame0");
    ASSERT_EQ(title(0), L"OTHER SPELL");
    press(RC::Input::MIDDLE_MOUSE_BUTTON);
    ASSERT_TRUE(ini().find("Slot1 = F5, \n") != std::string::npos);
    tick();
    ASSERT_EQ(title(0), L"EMPTY SLOT");
    ASSERT_EQ(text(0), L"Key: F5\nRight click: Choose spell\nMiddle click: Clear spell");
}

TEST_F(Setup, picks_an_unlocked_spell_from_the_list)
{
    const auto loads = Fake::game.asset_loads;
    Fake::game.hovered_frame = Fake::game.named.at(L"SpellBarFrame1");
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    ASSERT_EQ(Fake::game.rows.size(), 2u) << lines();
    ASSERT_TRUE(Fake::game.tooltips.size() == 4 && Fake::records.at(Fake::object_field(Fake::game.rows[0], 16)).visibility.empty());
    ASSERT_TRUE(member(Fake::game.rows[1], L"SpellData").read<UObject*>(L"ObjectProperty") == Fake::game.spell && member(Fake::game.rows[1], L"bUnlocked").boolean());
    ASSERT_TRUE(Fake::game.class_loads == 2 && Fake::game.log.empty() && ini().find("Slot2 = F6,\n") != std::string::npos);
    Fake::game.hovered_frame = Fake::game.rows[1];
    press(RC::Input::LEFT_MOUSE_BUTTON);
    ASSERT_TRUE(ini().find("Slot2 = F6, TestSpell\n") != std::string::npos);
    ASSERT_TRUE(Fake::logged("Configuration saved from the in-game panel.") && Fake::logged("Configuration accepted.") && Fake::logged("Spell bar released.") && Fake::game.log.empty());
    tick();
    ASSERT_TRUE(Fake::logged("Spell bar built.") && Fake::logged("Spell bar visible.") && Fake::game.log.empty());
    ASSERT_EQ(Fake::game.asset_loads, loads);
    ASSERT_TRUE(Fake::records.at(Fake::game.spell).roots == 1 && Fake::records.at(Fake::game.other).roots == 1 && Fake::records.at(Fake::game.locked_spell).roots == 1);
}

TEST_F(Setup, closes_the_list_without_a_pick)
{
    Fake::game.hovered_frame = frame0;
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    ASSERT_TRUE(Fake::game.rows.size() == 2 && widget(L"SpellBarPickerSize").visibility == L"Visible");
    Fake::game.hovered_frame = nullptr;
    press(RC::Input::LEFT_MOUSE_BUTTON);
    ASSERT_TRUE(widget(L"SpellBarPickerSize").visibility == L"Collapsed" && Fake::game.log.empty() && ini().find("Slot1 = F5, TestSpell\n") != std::string::npos);
    Fake::game.hovered_frame = frame0;
    const auto constructions = Fake::game.constructions;
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    ASSERT_TRUE(widget(L"SpellBarPickerSize").visibility == L"Visible" && Fake::game.constructions == constructions && Fake::game.rows.size() == 2);
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    ASSERT_TRUE(widget(L"SpellBarPickerSize").visibility == L"Collapsed" && Fake::game.log.empty());
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", false);
    tick();
    ASSERT_TRUE(widget(L"SpellBarPickerSize").visibility == L"Collapsed" && root_visibility() == L"HitTestInvisible");
    idle(10);
}

TEST_F(Setup, ignores_clicks_away_from_the_bar_or_with_a_modifier_held)
{
    Fake::game.hovered_frame = nullptr;
    press(RC::Input::RIGHT_MOUSE_BUTTON);
    press(RC::Input::MIDDLE_MOUSE_BUTTON);
    Fake::game.hovered_frame = frame0;
    press(RC::Input::RIGHT_MOUSE_BUTTON, { RC::Input::CONTROL });
    ASSERT_TRUE(Fake::game.log.empty());
    ASSERT_TRUE(ini().find("Slot1 = F5, TestSpell\n") != std::string::npos);
    ASSERT_EQ(Fake::game.tooltips.size(), 4u);
}

TEST_F(Setup, ignores_clicks_without_the_cursor)
{
    Fake::game.hovered_frame = frame0;
    member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", false);
    tick();
    ASSERT_EQ(root_visibility(), L"HitTestInvisible");
    press(RC::Input::MIDDLE_MOUSE_BUTTON);
    ASSERT_TRUE(Fake::game.log.empty());
    ASSERT_TRUE(ini().find("Slot1 = F5, TestSpell\n") != std::string::npos);
}

TEST_F(Setup, follows_the_cursor_and_the_wheel)
{
    auto gameplay = gameplay_mode();
    input_mode(Fake::game.radial_mode);
    tick();
    ASSERT_EQ(Fake::records.at(root).visibility, L"Hidden");
    input_mode(gameplay);
    tick();
    ASSERT_EQ(Fake::records.at(root).visibility, L"Visible");
    Fake::game.hovered_frame = nullptr;
    member(Fake::game.controller, L"bShowMouseCursor").write<bool>(L"BoolProperty", false);
    tick();
    ASSERT_EQ(Fake::records.at(root).visibility, L"HitTestInvisible");
    idle(10);
}

TEST_F(Setup, describes_a_slot_without_a_key)
{
    settings.spell_bar.slots[3] = {};
    configure();
    tick();
    ASSERT_EQ(text(3), L"No key\nRight click: Choose spell\nMiddle click: Clear spell");
}

TEST_F(Setup, reuses_the_loaded_tooltip_class_on_rebuild)
{
    settings.spell_bar.scale = 1.2f;
    configure();
    tick();
    ASSERT_EQ(Fake::game.tooltips.size(), 8u);
    ASSERT_EQ(Fake::game.class_loads, 1u);
}
}
