#pragma once
#include "Player.hpp"
#include <cstdint>

namespace Mods
{
enum Press : std::uint32_t
{
    right_click = 1u << 8,
    middle_click = 1u << 9,
    left_click = 1u << 10,
};

struct Frame
{
    Player* player{};
    UObject* controller{};
    UWorld* world{};
    UObject* outer{};
    std::uint32_t presses{};
    std::uint64_t now{};
    bool gameplay{};
    bool cursor{};
    bool shown{};
};
}
