#pragma once
#include "../Configuration.hpp"
#include "../game/Frame.hpp"
#include "../game/FunctionHook.hpp"
#include "../game/Widgets.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace Mods
{
class Slot
{
public:
    Slot(UObject* tree, UObject* panel, const Settings::SpellBar& config, int index, const std::wstring& suffix, const Widgets::Style& style, UObject* background, UObject* tooltip_class, UObject* owner, Player& player);
    UObject* asset() const { return m_asset; }
    bool hovered() { return m_hovered.get(); }
    void show_cooldown(bool on);
    void set_cooldown(double scale, int seconds);

private:
    UObject* m_asset;
    UObject* m_border;
    Retained<bool> m_hovered;
    UObject* m_layers;
    UObject* m_shade;
    Retained<std::wstring> m_shade_visibility;
    Retained<double> m_shade_scale;
    Widgets::Text m_seconds;
    int m_shown_seconds = -1;
};

class Bar
{
public:
    Bar(const Settings::SpellBar& config, UObject* outer, UObject* owner, Player& player, std::uint64_t generation);
    void show(bool on, bool for_setup);
    void release();
    bool visible() const { return m_visible; }
    bool hit_testable() const { return m_hit_testable; }
    bool hovered() { return m_hovered.get(); }
    Slot& slot(int index) { return m_slots[index]; }
    void open_picker(Player& player);
    void close_picker();
    bool picker_open() const { return m_picker_shown; }
    UObject* picked();

private:
    struct Row
    {
        UObject* spell;
        Retained<bool> hovered;
    };
    UObject* m_root;
    FWeakObjectPtr m_root_reference;
    Call m_detach;
    Retained<std::wstring> m_visibility;
    Retained<bool> m_hovered;
    UObject* m_tree;
    UObject* m_canvas;
    Widgets::Style m_style;
    UObject* m_owner;
    std::vector<Slot> m_slots;
    std::optional<Retained<std::wstring>> m_picker_visibility;
    std::vector<Row> m_rows;
    bool m_picker_shown{};
    bool m_visible{};
    bool m_hit_testable{};
};

class Setup
{
public:
    void frame(Bar& bar, const Settings::SpellBar& config, const Frame& current);
    bool active() const { return m_active; }
    std::optional<Edit> take_edit();

private:
    bool m_active{};
    int m_picker_slot = -1;
    std::optional<Edit> m_edit;
};

class Cooldowns
{
public:
    void watch(std::uint64_t now, std::uint64_t window)
    {
        m_timers.poll_until = now + window;
        m_timers.next_poll = 0;
    }
    bool poll(Bar& bar, const Settings::SpellBar& config, const Frame& current);
    void update(Bar& bar, const Settings::SpellBar& config, const Frame& current);

private:
    struct MapLayout { UClass* pawn_class{}; int magic{}; int map{}; int value{}; int stride{}; };
    struct Timers { std::uint64_t poll_until{}; std::uint64_t next_poll{}; std::uint64_t next_update{}; };
    MapLayout m_map;
    Timers m_timers;
    std::array<float, 8> m_last_cast{};
    std::array<bool, 8> m_cast_seen{};
    std::array<bool, 8> m_running{};
    std::array<double, 8> m_end{};
    std::array<float, 8> m_duration{};
    std::optional<Retained<double>> m_clock;

    void resolve_map(UObject* pawn, std::uint64_t now);
    void start(Bar& bar, int index, const Frame& current);
    void advance(Bar& bar, const Settings::SpellBar& config, std::uint64_t now);
};

class SpellBar
{
public:
    void configure(const Settings::SpellBar& config);
    bool wants_step(const Frame& frame) const;
    void step(const Frame& frame);
    void sample(const Frame& frame);
    std::optional<Edit> take_edit() { return m_setup.take_edit(); }
    void stop();

private:
    Settings::SpellBar m_config;
    bool m_display_dirty{};
    std::uint64_t m_generation{};
    std::optional<Bar> m_bar;
    Setup m_setup;
    Cooldowns m_cooldowns;
    std::optional<FunctionHook> m_cast_hooks[3];
    std::uint64_t m_cast_window{};

    void activate(int index, const Frame& frame);
};
}
