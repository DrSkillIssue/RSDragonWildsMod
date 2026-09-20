#include "Configuration.hpp"
#include <cstdio>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>

namespace Mods
{
const Container known_containers[6] = {
    { "CompostBucket", L"/Game/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_Bucket_Compost.ITEM_Bucket_Compost" },
    { "WoodWateringCan", L"/Game/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_WateringCan_Wood.ITEM_WateringCan_Wood" },
    { "BronzeWateringCan", L"/Game/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_WateringCan_Bronze.ITEM_WateringCan_Bronze" },
    { "SteelWateringCan", L"/Game/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_WateringCan_Steel.ITEM_WateringCan_Steel" },
    { "AdamantWateringCan", L"/UmbralSands/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_WateringCan_Adamant.ITEM_WateringCan_Adamant" },
    { "RuneWateringCan", L"/ScornedWilderness/Gameplay/Character/Player/Equipment/Held/Bucket/ITEM_WateringCan_Rune.ITEM_WateringCan_Rune" },
};

namespace
{
struct Modifier { const wchar_t* name; RC::Input::ModifierKey key; };
constexpr Modifier modifier_names[] = { { L"Ctrl", RC::Input::CONTROL }, { L"Shift", RC::Input::SHIFT }, { L"Alt", RC::Input::ALT } };
}

Configuration::Configuration(const std::filesystem::path& directory) : m_path(directory / "mods.ini") {}

bool Configuration::poll(std::uint64_t now)
{
    if (now < m_next_check) return false;
    m_next_check = now + 2000;
    std::error_code error;
    const auto modified = std::filesystem::last_write_time(m_path, error);
    if (error || modified == m_modified) return false;
    m_modified = modified;
    std::ifstream file(m_path);
    const std::string bytes((std::istreambuf_iterator<char>(file)), {});
    std::wstring text(bytes.begin(), bytes.end());
    Settings settings;
    struct Flag { const wchar_t* section; const wchar_t* key; bool* target; const char* error; };
    const Flag flags[] = {
        { L"Debug", L"Enabled", &settings.debug, "Debug.Enabled must be 0 or 1" },
        { L"SpellBar", L"Enabled", &settings.spell_bar.enabled, "SpellBar.Enabled must be 0 or 1" },
        { L"Containers", L"Enabled", &settings.containers.enabled, "Containers.Enabled must be 0 or 1" },
    };
    try
    {
        RC::Ini::Parser parser;
        parser.parse(text);
        for (const auto& flag : flags)
        {
            auto section = parser.section(flag.section);
            if (!section) continue;
            auto entry = section->key_value_pairs.find(flag.key);
            if (entry == section->key_value_pairs.end()) continue;
            if (!entry->second.is_valid_bool()) throw std::runtime_error(flag.error);
            *flag.target = entry->second.get_bool_value();
        }
        if (auto bar = parser.section(L"SpellBar"))
            for (const auto& [key, value] : bar->key_value_pairs)
            {
                if (key == L"Scale")
                {
                    if (!value.is_valid_float() || value.get_float_value() < 0.5f || value.get_float_value() > 2.f) throw std::runtime_error("SpellBar.Scale must be from 0.5 to 2");
                    settings.spell_bar.scale = value.get_float_value();
                }
                if (key.size() != 5 || !key.starts_with(L"Slot") || key[4] < L'1' || key[4] > L'8') continue;
                const std::size_t index = key[4] - L'1';
                if (settings.spell_bar.slots.size() <= index) settings.spell_bar.slots.resize(index + 1);
                auto& slot = settings.spell_bar.slots[index];
                const auto& line = value.get_string_value();
                const auto comma = line.find(L',');
                const auto spell = line.find_first_not_of(L' ', comma + 1);
                if (comma != std::wstring::npos && spell != std::wstring::npos) slot.spell = line.substr(spell);
                slot.key = line.substr(0, comma);
                if (slot.key.empty()) continue;
                std::size_t begin = 0, count = 0;
                for (auto plus = slot.key.find(L'+'); plus != std::wstring::npos; begin = plus + 1, plus = slot.key.find(L'+', begin))
                {
                    const auto part = slot.key.substr(begin, plus - begin);
                    const Modifier* found = nullptr;
                    for (const auto& modifier : modifier_names)
                        if (part == modifier.name) found = &modifier;
                    if (!found) throw std::runtime_error("SpellBar slot modifiers are Ctrl+, Shift+, Alt+");
                    slot.modifiers.at(count++) = found->key;
                }
                slot.code = RC::Input::string_to_key(slot.key.substr(begin));
            }
        if (auto containers = parser.section(L"Containers"))
            for (const auto& [key, value] : containers->key_value_pairs)
            {
                if (key == L"Enabled") continue;
                const Container* known = nullptr;
                for (const auto& container : known_containers)
                    if (key == std::wstring(container.name, container.name + std::char_traits<char>::length(container.name))) known = &container;
                if (!known) throw std::runtime_error("Containers has an unknown item. Use CompostBucket, WoodWateringCan, BronzeWateringCan, SteelWateringCan, AdamantWateringCan, RuneWateringCan");
                auto& item = settings.containers.items.emplace_back();
                item.name = known->name;
                const auto& line = value.get_string_value();
                const std::string narrow(line.begin(), line.end());
                int consumed = 0;
                if (std::sscanf(narrow.c_str(), "%d , %d %n", &item.capacity, &item.per_use, &consumed) != 2 || consumed != static_cast<int>(narrow.size()) || item.capacity < 1 || item.per_use < 1)
                    throw std::runtime_error("Containers items are 'capacity, per use' with whole numbers above 0");
            }
    }
    catch (const std::exception& e) { m_error = e.what(); return true; }
    m_settings = std::move(settings);
    m_error.clear();
    return true;
}

void Configuration::apply(const Edit& edit)
{
    m_settings.spell_bar.slots[edit.slot].spell = edit.spell;
    m_error.clear();
    std::ofstream file(m_path, std::ios::trunc);
    file << std::format("[Debug]\nEnabled = {:d}\n\n[SpellBar]\nEnabled = {:d}\nScale = {}\n",
        m_settings.debug, m_settings.spell_bar.enabled, m_settings.spell_bar.scale);
    int index = 0;
    for (const auto& slot : m_settings.spell_bar.slots)
        file << std::format("Slot{} = {}, {}\n", ++index, std::string(slot.key.begin(), slot.key.end()), std::string(slot.spell.begin(), slot.spell.end()));
    file << std::format("\n[Containers]\nEnabled = {:d}\n", m_settings.containers.enabled);
    for (const auto& item : m_settings.containers.items)
        file << std::format("{} = {}, {}\n", item.name, item.capacity, item.per_use);
    file.close();
    std::error_code error;
    m_modified = std::filesystem::last_write_time(m_path, error);
}
}
