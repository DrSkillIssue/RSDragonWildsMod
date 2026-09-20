#include <filesystem>
#include "SpellBarMod.hpp"

extern "C"
{
__declspec(dllexport) RC::CppUserModBase* start_mod()
{
    return new SpellBarMod(std::filesystem::path(RC::UE4SSProgram::get_program().get_mods_directory()) / L"DragonwildsSpellBar");
}

__declspec(dllexport) void uninstall_mod(RC::CppUserModBase* mod)
{
    delete mod;
}
}
