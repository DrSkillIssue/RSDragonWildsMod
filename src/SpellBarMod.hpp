#pragma once
#include "CppUserModBase.hpp"
#include "Configuration.hpp"
#include "containers/Containers.hpp"
#include "game/Frame.hpp"
#include "game/FunctionHook.hpp"
#include "game/Player.hpp"
#include "spellbar/SpellBar.hpp"
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>

class SpellBarMod final : public RC::CppUserModBase
{
public:
    explicit SpellBarMod(const std::filesystem::path& directory);
    ~SpellBarMod() override;
    void on_unreal_init() override;
    void configure(const Mods::Settings& settings);
    void tick(std::uint64_t now);

private:
    Mods::Configuration m_configuration;
    Mods::Player m_player;
    Mods::SpellBar m_spell_bar;
    Mods::Containers m_containers;
    std::optional<Mods::FunctionHook> m_restart;
    RC::Unreal::Hook::GlobalCallbackId m_tick_callback{};
    std::atomic<std::uint32_t> m_presses{};

    void configure();
    void bind(RC::Unreal::UObject* controller);
    void release();
};
