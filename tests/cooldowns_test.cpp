#include "fake/Fixture.hpp"

namespace
{
struct Cooldowns : Fixture
{
    UObject* shade0;
    UObject* seconds0;

    void SetUp() override
    {
        bind(0, L"TestSpell", L"F5", static_cast<RC::Input::Key>(0x74));
        start();
        shade0 = Fake::game.named.at(L"SpellBarShade0");
        seconds0 = Fake::game.named.at(L"SpellBarSeconds0");
    }

    void cast_from_key(float map_value, double world_time)
    {
        Fake::cast_entry(0, Fake::game.spell, map_value);
        Fake::game.world_time = world_time;
        Fake::time += 5000;
        press(static_cast<RC::Input::Key>(0x74));
        Fake::call(Fake::game.magic, L"Multicast_SendPayloadForSpellCasting");
        tick();
        ASSERT_EQ(root_visibility(), L"HitTestInvisible");
    }

    const std::wstring& shade() { return Fake::records.at(shade0).visibility; }
    const std::wstring& seconds() { return Fake::records.at(seconds0).display_text; }
};

TEST_F(Cooldowns, watches_the_map_after_a_prepare_until_the_cast_lands)
{
    Fake::call(Fake::game.magic, L"Server_NotifyPreparingSpell");
    tick();
    Fake::time += 6000;
    Fake::cast_entry(0, Fake::game.spell, 100);
    Fake::game.world_time = 110;
    tick();
    ASSERT_EQ(shade(), L"HitTestInvisible");
    Fake::time += 5000;
    Fake::game.world_time = 200;
    tick();
    ASSERT_EQ(shade(), L"Collapsed");
    Fake::time += 1000;
    Fake::cast_entry(0, Fake::game.spell, 300);
    Fake::game.world_time = 305;
    idle(100);
    ASSERT_EQ(shade(), L"Collapsed");
    Fake::call(Fake::game.magic, L"HandleOnSpellCastingAnimationFinished");
    tick();
    ASSERT_EQ(shade(), L"HitTestInvisible");
}

TEST_F(Cooldowns, watches_from_the_cast_payload_until_the_animation_ends)
{
    Fake::call(Fake::game.magic, L"Multicast_SendPayloadForSpellCasting");
    tick();
    Fake::time += 1700;
    Fake::cast_entry(0, Fake::game.spell, 100);
    Fake::game.world_time = 101;
    tick();
    ASSERT_EQ(shade(), L"HitTestInvisible");
    Fake::call(Fake::game.magic, L"HandleOnSpellCastingAnimationFinished");
    tick();
    Fake::time += 1500;
    Fake::game.world_time = 200;
    tick();
    ASSERT_EQ(shade(), L"Collapsed");
    idle(100);
}

TEST_F(Cooldowns, ignores_the_wheel_without_a_cast_event)
{
    auto gameplay = gameplay_mode();
    input_mode(Fake::game.radial_mode);
    Fake::cast_entry(0, Fake::game.spell, 100);
    Fake::game.world_time = 110;
    tick();
    ASSERT_EQ(root_visibility(), L"Hidden");
    Fake::time += 5000;
    idle(100);
    ASSERT_EQ(shade(), L"Collapsed");
    input_mode(gameplay);
}

TEST_F(Cooldowns, starts_a_cooldown_from_a_key_cast_and_drains_it)
{
    idle(1000);
    cast_from_key(100, 110);
    ASSERT_TRUE(Fake::game.log.empty());
    ASSERT_TRUE(shade() == L"HitTestInvisible" && seconds() == L"15");
    ASSERT_EQ(Fake::game.last_scale_y, 15.0 / 25);
    Fake::time += 50; Fake::game.world_time = 120;
    const auto allocations = Fake::heap_allocations, lookups = Fake::game.lookups;
    tick();
    ASSERT_TRUE(Fake::heap_allocations == allocations && Fake::game.lookups == lookups);
    ASSERT_TRUE(Fake::game.last_scale_y == 5.0 / 25 && seconds() == L"5");
    Fake::time += 50; Fake::game.world_time = 126;
    tick();
    ASSERT_TRUE(shade() == L"Collapsed" && Fake::records.at(seconds0).visibility == L"Collapsed");
    Fake::time += 5000;
    idle(1000);
}

TEST_F(Cooldowns, sees_a_cast_made_from_the_wheel)
{
    cast_from_key(100, 110);
    Fake::time += 5000; Fake::game.world_time = 200;
    tick();
    ASSERT_EQ(shade(), L"Collapsed");
    Fake::time += 5000;
    idle(100);
    auto gameplay = gameplay_mode();
    input_mode(Fake::game.radial_mode);
    Fake::cast_entry(0, Fake::game.spell, 200);
    Fake::game.world_time = 205;
    Log::debug = true;
    Fake::call(Fake::game.magic, L"Server_NotifyPreparingSpell");
    tick();
    ASSERT_EQ(root_visibility(), L"Hidden");
    ASSERT_TRUE(Fake::game.last_scale_y == 20.0 / 25 && seconds() == L"20");
    ASSERT_TRUE(Fake::logged("Cast event received from the game, watching for 10000 ms."));
    ASSERT_TRUE(Fake::logged("Cast seen for slot 1: map value 200.00, duration 25.00, clock 205.00"));
    ASSERT_TRUE(Fake::logged("Spell bar hidden.") && Fake::game.log.empty());
    Log::debug = false;
    input_mode(gameplay);
    Fake::time += 50; Fake::game.world_time = 300;
    tick();
    ASSERT_EQ(root_visibility(), L"HitTestInvisible");
    ASSERT_EQ(shade(), L"Collapsed");
}

TEST_F(Cooldowns, restores_a_running_cooldown_after_a_restart)
{
    cast_from_key(100, 110);
    Fake::cast_entry(0, Fake::game.spell, 290);
    Fake::game.world_time = 300;
    Fake::time += 5000;
    Fake::end_play(reinterpret_cast<AActor*>(Fake::game.controller), static_cast<EEndPlayReason>(0));
    restart();
    tick(1000);
    ASSERT_TRUE(Fake::rooted());
    tick();
    auto shade1 = Fake::game.named.at(L"SpellBarShade0");
    ASSERT_TRUE(shade1 != shade0 && Fake::records.at(shade1).visibility == L"HitTestInvisible" && Fake::game.last_scale_y == 15.0 / 25);
    Fake::game.world_time = 400; Fake::time += 50;
    tick();
    ASSERT_EQ(Fake::records.at(shade1).visibility, L"Collapsed");
}
}
