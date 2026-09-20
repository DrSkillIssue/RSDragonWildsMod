#pragma once
#include "Fake.hpp"
#include "Log.hpp"
#include "SpellBarMod.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <optional>
#include <string>

struct Fixture : ::testing::Test
{
    Settings settings;
    std::filesystem::path directory = std::filesystem::temp_directory_path() / "dragonwilds-fixture";
    std::optional<SpellBarMod> mod;

    Fixture()
    {
        Fake::reset();
        Fake::setup();
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        mod.emplace(directory);
        mod->on_unreal_init();
        Fake::game.log.clear();
        for (int i = 0; i < 4; ++i) settings.spell_bar.slots.push_back({ L"F" + std::to_wstring(5 + i), static_cast<RC::Input::Key>(0x74 + i), {}, {} });
    }

    ~Fixture() { mod.reset(); }

    void configure() { settings.debug = Log::debug; mod->configure(settings); Fake::game.log.clear(); }
    void tick() { mod->tick(Fake::time); }
    void tick(std::uint64_t advance) { Fake::time += advance; tick(); }
    void restart() { Fake::call(Fake::game.controller, L"ClientRestart"); }
    void hover(UObject* row)
    {
        Fake::game.hovered_frame = row;
        member(row, L"bIsTooltipSpawned").write(L"BoolProperty", true);
        member(row, L"TooltipWidgetClass").write(L"ClassProperty", Fake::game.perk_tooltip);
    }
    void end_world() { Fake::records.at(Fake::game.controller).serial = 0; tick(); }

    void start()
    {
        configure();
        restart();
        tick();
        ASSERT_TRUE(Fake::rooted()) << lines();
        ASSERT_EQ(root_visibility(), L"HitTestInvisible");
        tick();
        Fake::game.log.clear();
    }

    std::string lines()
    {
        std::string joined;
        for (const auto& line : Fake::game.log) joined += line;
        return joined;
    }

    void idle(int frames)
    {
        const auto calls = Fake::game.calls, lookups = Fake::game.lookups, scans = Fake::game.scans;
        const auto allocations = Fake::heap_allocations;
        for (int i = 0; i < frames; ++i) tick();
        ASSERT_TRUE(Fake::game.calls == calls && Fake::game.lookups == lookups && Fake::game.scans == scans && Fake::heap_allocations == allocations);
    }

    void bind(int slot, const wchar_t* spell, const wchar_t* key, RC::Input::Key code, RC::Input::ModifierKeyArray modifiers = {})
    {
        settings.spell_bar.slots[slot] = { key, code, modifiers, spell };
    }

    void press(RC::Input::Key key, RC::Input::ModifierKeyArray held = {}) { Fake::press(key, held); tick(); }

    void input_mode(UObject* mode)
    {
        member(Fake::game.controller, L"CurrentInputMode").write(L"ObjectProperty", mode);
    }

    UObject* gameplay_mode() { return member(Fake::game.controller, L"CurrentInputMode").read<UObject*>(L"ObjectProperty"); }
    const Fake::Record& widget(const wchar_t* name) { return Fake::records.at(Fake::game.named.at(name)); }
    std::wstring root_visibility() { return Fake::rooted() ? Fake::records.at(Fake::rooted()).visibility : L""; }
};
