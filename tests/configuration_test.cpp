#include "fake/Fixture.hpp"
#include "Configuration.hpp"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
struct ModsIni : ::testing::Test
{
    std::filesystem::path directory = std::filesystem::temp_directory_path() / "dragonwilds-config-test";
    Configuration configuration;

    ModsIni() : configuration((std::filesystem::remove_all(directory), std::filesystem::create_directories(directory), directory)) {}
    ~ModsIni() { std::filesystem::remove_all(directory); }

    void write(const char* body, int touch_seconds = 0)
    {
        std::ofstream(directory / "mods.ini", std::ios::trunc) << body;
        if (touch_seconds) std::filesystem::last_write_time(directory / "mods.ini", std::filesystem::last_write_time(directory / "mods.ini") + std::chrono::seconds(touch_seconds));
    }

    std::string read()
    {
        std::ifstream in(directory / "mods.ini");
        return std::string(std::istreambuf_iterator<char>(in), {});
    }
};

constexpr const char* full_file =
    "[Debug]\nEnabled = 1\n\n[SpellBar]\nEnabled = 1\nScale = 1.25\nSlot1 = Ctrl+Q, USD_Spell\nSlot2 = F5, USD_Fish\nSlot3 = ,\nSlot4 = Shift+Alt+K, \n\n"
    "[Containers]\nEnabled = 1\nRuneWateringCan = 300, 25 \nCompostBucket = 5000,50\n";

TEST_F(ModsIni, parses_a_full_file)
{
    ASSERT_TRUE(!configuration.poll(1000) && std::strcmp(configuration.error(), "") == 0);
    write(full_file);
    ASSERT_TRUE(configuration.poll(3000) && configuration.settings().debug);
    const auto& bar = configuration.settings().spell_bar;
    ASSERT_TRUE(bar.enabled && bar.slots.size() == 4 && bar.scale == 1.25f);
    ASSERT_TRUE((bar.slots[0] == Settings::Slot{ L"Ctrl+Q", static_cast<RC::Input::Key>(0x51), { RC::Input::CONTROL }, L"USD_Spell" }));
    ASSERT_TRUE((bar.slots[1] == Settings::Slot{ L"F5", static_cast<RC::Input::Key>(0x74), {}, L"USD_Fish" }));
    ASSERT_TRUE((bar.slots[2] == Settings::Slot{}));
    ASSERT_TRUE((bar.slots[3] == Settings::Slot{ L"Shift+Alt+K", static_cast<RC::Input::Key>(0x4B), { RC::Input::SHIFT, RC::Input::ALT }, {} }));
    const auto& items = configuration.settings().containers;
    ASSERT_TRUE(items.enabled && items.items.size() == 2);
    ASSERT_TRUE((items.items[0] == Settings::Item{ "CompostBucket", 5000, 50 }) && (items.items[1] == Settings::Item{ "RuneWateringCan", 300, 25 }));
    ASSERT_TRUE(!configuration.poll(4000) && !configuration.poll(5001));
}

TEST_F(ModsIni, applies_spell_edits_and_writes_them_back)
{
    write(full_file);
    ASSERT_TRUE(configuration.poll(3000));
    configuration.apply({ 2, L"USD_Other" });
    ASSERT_TRUE(read().find("Slot3 = , USD_Other\n") != std::string::npos && configuration.settings().spell_bar.slots[2].spell == L"USD_Other");
    configuration.apply({ 0, {} });
    ASSERT_TRUE(configuration.settings().spell_bar.slots[0].spell.empty());
    ASSERT_EQ(read(), "[Debug]\nEnabled = 1\n\n[SpellBar]\nEnabled = 1\nScale = 1.25\nSlot1 = Ctrl+Q, \nSlot2 = F5, USD_Fish\nSlot3 = , USD_Other\nSlot4 = Shift+Alt+K, \n\n[Containers]\nEnabled = 1\nCompostBucket = 5000, 50\nRuneWateringCan = 300, 25\n");
    ASSERT_TRUE(!configuration.poll(8000));
}

TEST_F(ModsIni, rejects_invalid_files_and_keeps_the_previous_configuration)
{
    write(full_file);
    ASSERT_TRUE(configuration.poll(3000));
    const std::pair<const char*, const char*> rejected[] = {
        { "[SpellBar]\nSlot1 = Super+Q,\n", "Ctrl+, Shift+, Alt+" },
        { "[SpellBar]\nSlot1 = F25,\n", "Key not found: F25" },
        { "[SpellBar]\nSlot1 = Ctrl+,\n", "Key not found: " },
        { "[SpellBar]\nScale = 3\n", "0.5 to 2" },
        { "[SpellBar]\nEnabled = yes\n", "0 or 1" },
        { "[Containers]\nCompostBucket = 10\n", "whole numbers" },
        { "[Containers]\nCompostBucket = ten, 1\n", "whole numbers" },
        { "[Containers]\nCompostBucket = 0, 1\n", "whole numbers" },
        { "[Containers]\nCompostBucket = 5, 1 x\n", "whole numbers" },
        { "[Containers]\nGoldWateringCan = 10, 1\n", "unknown item" },
        { "[SpellBar]\nSlot1 F5\n", "Syntax error" },
    };
    std::uint64_t check_time = 20000;
    for (auto [body, error] : rejected)
    {
        write(body, 2);
        check_time += 3000;
        ASSERT_TRUE(configuration.poll(check_time) && std::strstr(configuration.error(), error)) << body << configuration.error();
        ASSERT_TRUE(configuration.settings().spell_bar.slots.size() == 4 && configuration.settings().containers.items.size() == 2);
    }
    configuration.apply({ 2, L"USD_Other" });
    ASSERT_TRUE(std::strcmp(configuration.error(), "") == 0 && read().find("Slot3 = , USD_Other\n") != std::string::npos);
}

TEST_F(ModsIni, reloads_a_changed_file)
{
    write(full_file);
    ASSERT_TRUE(configuration.poll(3000));
    write("[SpellBar]\nScale = 0.5\nSlot1 = F5,\n", 4);
    ASSERT_TRUE(configuration.poll(90000));
    const auto& settings = configuration.settings();
    ASSERT_TRUE(settings.spell_bar.scale == 0.5f && settings.spell_bar.slots.size() == 1 && settings.spell_bar.slots[0].code == 0x74 && !settings.debug && settings.containers.items.empty());
}
}
