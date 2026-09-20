#include "Fake.hpp"
#include "Log.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

size_t Fake::heap_allocations{};
void* operator new(size_t size) { ++Fake::heap_allocations; if (auto p = std::malloc(size ? size : 1)) return p; throw std::bad_alloc(); }
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }
void operator delete[](void* p, size_t) noexcept { std::free(p); }

namespace Fake
{
State game;
std::unordered_map<const void*, Record> records;
std::vector<std::unique_ptr<unsigned char[]>> storage;
std::vector<UObject*> objects{nullptr};
std::vector<std::wstring> names{L"None"};
std::function<void(Hook::TCallbackIterationData<void>&, UEngine*, float, bool)> engine_tick;
std::uint64_t time = 1000;

Object::Object(const std::wstring& name, UClass* type)
{
    auto bytes = std::make_unique<unsigned char[]>(2048);
    pointer = reinterpret_cast<UObject*>(bytes.get());
    storage.push_back(std::move(bytes));
    Record record;
    record.name = name; record.type = type; record.id = static_cast<int>(objects.size());
    records.emplace(pointer, std::move(record));
    objects.push_back(pointer);
    FName(name.c_str(), add_name, nullptr);
}

void field(UStruct* owner, const wchar_t* name, const wchar_t* kind, int offset, int size, UScriptStruct* structure, FProperty* inner)
{
    Object item(name);
    auto p = reinterpret_cast<FProperty*>(item.pointer);
    auto& r = records.at(p);
    r.kind = kind; r.offset = offset; r.size = size; r.structure = structure; r.inner = inner;
    r.next = records.at(owner).children;
    records.at(owner).children = p;
    FName(kind, add_name, nullptr);
}

UFunction* function(const wchar_t* name, int size, std::initializer_list<std::tuple<const wchar_t*, const wchar_t*, int, int>> parameters)
{
    Object item(name);
    auto f = reinterpret_cast<UFunction*>(item.pointer);
    records.at(f).parameter_size = size;
    game.functions.emplace(name, f);
    for (auto [n, k, o, s] : parameters) field(f, n, k, o, s);
    return f;
}

void call(UObject* object, const wchar_t* name)
{
    Call invocation(object, name);
    invocation.run();
}

UObject* object_field(UObject* owner, int offset)
{
    UObject* result;
    std::memcpy(&result, reinterpret_cast<unsigned char*>(owner) + offset, sizeof(result));
    return result;
}

UObject* asset(const wchar_t* path)
{
    Object item(path);
    game.paths.emplace(path, item.pointer);
    return item.pointer;
}

UObject* rooted()
{
    UObject* result{};
    for (auto& [pointer, record] : records)
        if (record.roots && record.type == widget_class) { assert(!result); result = const_cast<UObject*>(static_cast<const UObject*>(pointer)); }
    return result;
}

int mark() { return static_cast<int>(objects.size()); }

bool all_alive_since(int since)
{
    for (auto& [pointer, record] : records)
        if (record.collectable && record.id >= since && !record.alive) return false;
    return true;
}

void cast_entry(int index, UObject* key, float value)
{
    std::memcpy(game.cast_pairs + index * 16, &key, 8);
    std::memcpy(game.cast_pairs + index * 16 + 8, &value, 4);
    int count = index + 1;
    std::memcpy(game.cast_map + 8, &count, 4);
    std::memcpy(game.cast_map + 40, &count, 4);
    uint32_t bits = (1u << count) - 1;
    std::memcpy(game.cast_map + 16, &bits, 4);
}

void collect()
{
    std::vector<UObject*> pending;
    for (auto& [pointer, record] : records)
    {
        record.marked = false;
        if (record.roots || record.attached) pending.push_back(const_cast<UObject*>(static_cast<const UObject*>(pointer)));
    }
    while (!pending.empty())
    {
        auto object = pending.back(); pending.pop_back();
        auto& record = records.at(object);
        if (record.marked) continue;
        assert(record.alive); record.marked = true;
        if (record.outer) pending.push_back(record.outer);
        for (auto child : record.references) pending.push_back(child);
        if (record.type)
            for (auto field = records.at(record.type).children; field; field = records.at(field).next)
                if (records.at(field).kind == L"ObjectProperty")
                {
                    auto child = object_field(object, records.at(field).offset);
                    if (child) pending.push_back(child);
                }
    }
    for (auto& [pointer, record] : records)
        if (record.collectable && !record.marked) record.alive = false;
}

void reset()
{
    game = State{};
    for (auto& [pointer, record] : records) { record.alive = false; record.roots = 0; record.attached = false; }
    Log::debug = false;
    time += 10000;
}

void set_world_functions(bool alive)
{
    for (auto name : {L"GetOwningPlayer", L"OnSpellRadialChanged", L"SelectSlice", L"GetCurrentlySelectedSpellData"})
    {
        auto function = game.functions.at(name);
        records.at(function).alive = alive;
        for (auto field = records.at(function).children; field; field = records.at(field).next) records.at(field).alive = alive;
    }
}

void destroy_world_functions() { set_world_functions(false); }
void restore_world_functions() { set_world_functions(true); }

bool logged(const char* fragment)
{
    for (auto line = game.log.begin(); line != game.log.end(); ++line)
    {
        if (!std::strstr(line->c_str(), fragment)) continue;
        game.log.erase(line);
        return true;
    }
    return false;
}
}
