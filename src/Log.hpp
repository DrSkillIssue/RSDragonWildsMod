#pragma once
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#ifdef DW_DEV
#include <cassert>
#define DW_ASSERT(condition) assert(condition)
#else
#define DW_ASSERT(condition) ((void)0)
#endif

namespace RC::Output
{
__declspec(dllimport) void send(std::wstring_view content);
}

namespace Mods::Log
{
inline bool debug{};

template<class... Args> void write(std::format_string<Args...> format, Args&&... args)
{
    std::string line = "[DragonwildsSpellBar] ";
    std::format_to(std::back_inserter(line), format, std::forward<Args>(args)...);
    line += '\n';
    RC::Output::send(std::wstring(line.begin(), line.end()));
}
}

#define DW_LOG_DEBUG(...) do { if (::Mods::Log::debug) ::Mods::Log::write(__VA_ARGS__); } while (0)
