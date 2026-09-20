#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "Configuration.hpp"
#include "Reflection.hpp"

using namespace Mods;

namespace Fake
{
struct Record
{
    std::wstring name, kind;
    std::wstring display_text, visibility, brush;
    UClass* type{};
    UStruct* parent{};
    FField* next{};
    FField* children{};
    UScriptStruct* structure{};
    FProperty* inner{};
    FProperty* value_inner{};
    UWorld* world{};
    UObject* outer{};
    std::vector<UObject*> references;
    std::vector<std::pair<std::function<void(UnrealScriptFunctionCallableContext&, void*)>, void*>> post_hooks;
    uint32_t object_flags{};
    int offset{}, size{}, id{}, roots{}, serial{};
    EPropertyFlags flags = static_cast<EPropertyFlags>(0x80);
    unsigned short parameter_size{};
    bool alive = true, attached{}, collectable{}, marked{};
};

struct State
{
    std::unordered_map<std::wstring, UObject*> paths;
    std::unordered_map<std::wstring, UFunction*> functions;
    std::unordered_map<std::wstring, UObject*> named;
    std::unordered_map<std::wstring, UObject*> loadable;
    std::vector<UObject*> tooltips;
    std::vector<UObject*> rows;
    size_t class_loads{}, asset_loads{};
    size_t lookups{}, calls{}, scans{}, constructions{}, adds{}, removes{}, parameter_allocations{}, style_textures{};
    size_t serials{};
    std::wstring imported_text;
    int fail_after = -1;
    bool ready = true, rooting_works = true, clearing_works = true;
    UObject* viewport{}, *controller{}, *game_instance{}, *radial_mode{}, *casting_mode{}, *hovered_frame{};
    float last_width{}, last_height{};
    double last_icon_x{}, last_icon_y{}, last_scale_y{}, world_time{};
    int perk_count = 3;
    UObject* wheel{}, *wheel_template{}, *spellbook_wheel{}, *component{}, *spell{}, *other{}, *magic{};
    UObject* skill{}, *skill_default{}, *locked_spell{}, *locked_perk{}, *perk_component{};
    unsigned char perk_soft[3 * 40]{};
    UObject* assigned_spells[48]{};
    UObject* slices[12]{};
    bool wheel_available = true, selection_allowed = true;
    uint16_t selected_page{};
    uint8_t current_slot{};
    int selections{}, page_changes{};
    unsigned char mapping_data[8]{};
    UObject* player_input{};
    unsigned char cast_pairs[4 * 16]{};
    unsigned char* cast_map{};
    FScriptMapLayout map_layout{8, {12, 16, 12, {8, 16}}};
    std::vector<std::string> log;
    std::vector<std::uint64_t> callbacks;
    std::uint64_t callback_ids{}, tick_callback{}, end_play_callback{};
    RC::Input::KeySet key_set;
};

extern State game;
extern std::unordered_map<const void*, Record> records;
extern std::vector<std::unique_ptr<unsigned char[]>> storage;
extern std::vector<UObject*> objects;
extern std::vector<std::wstring> names;
extern std::function<void(AActor*, EEndPlayReason)> end_play;
extern std::function<void(Hook::TCallbackIterationData<void>&, UEngine*, float, bool)> engine_tick;
extern std::uint64_t time;
extern UClass* widget_class;
extern size_t heap_allocations;

struct Object
{
    UObject* pointer;
    Object(const std::wstring& name, UClass* type = nullptr);
};

void field(UStruct* owner, const wchar_t* name, const wchar_t* kind, int offset, int size, UScriptStruct* structure = nullptr, FProperty* inner = nullptr);
UFunction* function(const wchar_t* name, int size, std::initializer_list<std::tuple<const wchar_t*, const wchar_t*, int, int>> parameters);
void call(UObject* object, const wchar_t* name);
UObject* object_field(UObject* owner, int offset);
UObject* asset(const wchar_t* path);
UObject* rooted();
int mark();
bool all_alive_since(int since);
void cast_entry(int index, UObject* key, float value);
void collect();
void reset();
void setup();
void destroy_world_functions();
void restore_world_functions();
void press(RC::Input::Key key, RC::Input::ModifierKeyArray held = {});
bool logged(const char* fragment);
}
