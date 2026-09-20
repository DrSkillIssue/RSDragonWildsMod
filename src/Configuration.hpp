#pragma once
#include "UnrealAbi.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Mods
{
struct Settings
{
    struct Slot
    {
        std::wstring key;
        RC::Input::Key code{};
        RC::Input::ModifierKeyArray modifiers{};
        std::wstring spell;
        bool operator==(const Slot&) const = default;
    };
    struct SpellBar
    {
        bool enabled = true;
        float scale = 1;
        std::vector<Slot> slots;
        bool operator==(const SpellBar&) const = default;
    };
    struct Item
    {
        std::string name;
        int capacity{};
        int per_use{};
        bool operator==(const Item&) const = default;
    };
    struct Containers
    {
        bool enabled = true;
        std::vector<Item> items;
        bool operator==(const Containers&) const = default;
    };
    bool debug{};
    SpellBar spell_bar;
    Containers containers;
};

struct Container { const char* name; const wchar_t* asset; };
extern const Container known_containers[6];

struct Edit { int slot; std::wstring spell; };

class Configuration
{
public:
    explicit Configuration(const std::filesystem::path& directory);
    bool poll(std::uint64_t now);
    void apply(const Edit& edit);
    const Settings& settings() const { return m_settings; }
    const char* error() const { return m_error.c_str(); }

private:
    std::filesystem::path m_path;
    std::filesystem::file_time_type m_modified{};
    std::uint64_t m_next_check{};
    Settings m_settings;
    std::string m_error;
};
}
