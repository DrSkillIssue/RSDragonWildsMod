#include "Fake.hpp"
#include "Log.hpp"
#include "SpellBarMod.hpp"
#include <algorithm>
#include <cassert>
#include <cwchar>
#include <cwctype>
#include <stdexcept>

namespace Fake
{
namespace
{
std::uint32_t modifier_bits(const RC::Input::ModifierKeyArray& keys)
{
    std::uint32_t bits = 0;
    for (auto key : keys)
        if (key > 0 && key < RC::Input::MODIFIER_KEYS_MAX) bits |= 1u << key;
    return bits;
}
}

void press(RC::Input::Key key, RC::Input::ModifierKeyArray held)
{
    const auto found = game.key_set.key_data.find(key);
    if (found == game.key_set.key_data.end()) return;
    std::vector<std::function<void()>> callbacks;
    for (const auto& data : found->second)
        if (data.required_modifier_keys == modifier_bits(held)) callbacks.push_back(data.callback);
    for (const auto& callback : callbacks) callback();
}
}

namespace RC
{
CppUserModBase::CppUserModBase() = default;

CppUserModBase::~CppUserModBase()
{
    std::erase_if(Fake::game.key_set.key_data, [this](auto& entry)
    {
        std::erase_if(entry.second, [this](Input::KeyData& data)
        {
            auto event = static_cast<KeyDownEventData*>(data.custom_data2);
            if (data.custom_data != 2 || !event || event->mod != this) return false;
            delete event;
            return true;
        });
        return entry.second.empty();
    });
}

void CppUserModBase::register_keydown_event(Input::Key key, const Input::ModifierKeyArray& modifiers, const std::function<void()>& callback, uint8_t custom_data)
{
    auto& data = Fake::game.key_set.key_data[key].emplace_back();
    data.required_modifier_keys = Fake::modifier_bits(modifiers);
    data.callback = callback;
    data.custom_data = 2;
    data.custom_data2 = new KeyDownEventData{ custom_data, this };
    data.requires_modifier_keys = true;
}

UE4SSProgram& UE4SSProgram::get_program()
{
    static UE4SSProgram program;
    return program;
}

void UE4SSProgram::get_all_input_events(std::function<void(Input::KeySet&)> callback)
{
    callback(Fake::game.key_set);
}
}

RC::Input::Key RC::Input::string_to_key(const std::wstring& string)
{
    std::wstring lower = string;
    for (auto& c : lower) c = static_cast<wchar_t>(std::towlower(c));
    if (lower.size() == 1 && lower[0] >= L'a' && lower[0] <= L'z') return static_cast<Key>(0x41 + lower[0] - L'a');
    wchar_t* end{};
    const auto number = lower.size() > 1 && lower[0] == L'f' ? std::wcstol(lower.c_str() + 1, &end, 10) : 0;
    if (number >= 1 && number <= 24 && !*end) return static_cast<Key>(0x6F + number);
    throw std::runtime_error("string_to_key: Key not found: " + std::string(string.begin(), string.end()));
}

template<> class RC::Unreal::Hook::TCallbackIterationData<void> {};

void RC::Output::send(std::wstring_view content)
{
    Fake::game.log.emplace_back(content.begin(), content.end());
}

namespace RC::Unreal
{
namespace UObjectGlobals
{
UObject* FindObject(UClass*, UObject*, const wchar_t* path, bool, ObjectSearcher*)
{
    ++Fake::game.lookups;
    const auto found = Fake::game.paths.find(path);
    if (found == Fake::game.paths.end()) return nullptr;
    return found->second;
}

void FindAllOf(const wchar_t* class_name, std::vector<UObject*>& output)
{
    ++Fake::game.scans;
    if (std::wcscmp(class_name, L"GameViewportClient") == 0) output.push_back(Fake::game.viewport);
    if (std::wcscmp(class_name, L"SkillData") == 0) { output.push_back(Fake::game.skill_default); output.push_back(Fake::game.skill); }
    if (std::wcscmp(class_name, L"WBP_SurvivalSorcery_RadialSelector_C") == 0 && Fake::game.wheel_available)
    {
        output.push_back(Fake::game.wheel_template);
        output.push_back(Fake::game.spellbook_wheel);
        output.push_back(Fake::game.wheel);
    }
}

UObject* StaticConstructObject(const FStaticConstructObjectParameters& parameters)
{
    if (Fake::game.fail_after == 0) return nullptr;
    if (Fake::game.fail_after > 0) --Fake::game.fail_after;
    ++Fake::game.constructions;
    Fake::Object object(L"Widget", const_cast<UClass*>(parameters.type));
    auto& record = Fake::records.at(object.pointer);
    record.outer = parameters.outer;
    record.object_flags = parameters.flags;
    record.collectable = true;
    Fake::game.named[const_cast<FName&>(parameters.name).ToString()] = object.pointer;
    return object.pointer;
}
}

namespace Hook
{
GlobalCallbackId RegisterEngineTickPostCallback(std::function<void(TCallbackIterationData<void>&, UEngine*, float, bool)> callback, FCallbackOptions options)
{
    assert(options.OwnerModName == L"DragonwildsSpellBar" && !options.HookName.empty());
    Fake::engine_tick = std::move(callback);
    Fake::game.callbacks.push_back(++Fake::game.callback_ids);
    Fake::game.tick_callback = Fake::game.callback_ids;
    return Fake::game.callback_ids;
}

GlobalCallbackId RegisterEndPlayPreCallback(std::function<void(TCallbackIterationData<void>&, AActor*, EEndPlayReason)> callback, FCallbackOptions options)
{
    assert(options.OwnerModName == L"DragonwildsSpellBar" && !options.HookName.empty());
    Fake::end_play = [callback = std::move(callback)](AActor* actor, EEndPlayReason reason)
    {
        TCallbackIterationData<void> data;
        callback(data, actor, reason);
    };
    Fake::game.callbacks.push_back(++Fake::game.callback_ids);
    Fake::game.end_play_callback = Fake::game.callback_ids;
    return Fake::game.callback_ids;
}

bool UnregisterCallback(GlobalCallbackId id)
{
    auto& live = Fake::game.callbacks;
    const auto found = std::find(live.begin(), live.end(), id);
    if (found == live.end()) return false;
    live.erase(found);
    if (id == Fake::game.tick_callback) Fake::engine_tick = nullptr;
    if (id == Fake::game.end_play_callback) Fake::end_play = nullptr;
    return true;
}
}
}
