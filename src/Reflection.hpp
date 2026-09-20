#pragma once
#include "UnrealAbi.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace Mods
{
using namespace RC::Unreal;
constexpr auto find_name = static_cast<EFindName>(0);
constexpr auto add_name = static_cast<EFindName>(1);
struct Array { unsigned char* data; int count, capacity; };

inline std::string narrow(const wchar_t* text) { return std::string(text, text + std::wcslen(text)); }

struct Permanent {};
constexpr Permanent permanent;
inline FWeakObjectPtr weak(UObject* object);

inline FProperty* property(UStruct* type, const wchar_t* name)
{
    const FName wanted(name, find_name, nullptr);
    if (!wanted.none())
        for (; type; type = type->GetSuperStruct())
            for (auto field = type->GetChildProperties(); field; field = field->Next())
                if (field->GetFName() == wanted) return static_cast<FProperty*>(field);
    throw std::runtime_error("Required property is absent: " + narrow(name));
}

template<class T> struct Typed
{
    unsigned char* data{};
    T read() const { T value; std::memcpy(&value, data, sizeof(T)); return value; }
    void write(T value) const { std::memcpy(data, &value, sizeof(T)); }
};

struct Flag
{
    FBoolProperty* field{};
    unsigned char* data{};
    bool read() const { return field->GetPropertyValue(data); }
    void write(bool value) const { field->SetPropertyValue(data, value); }
};

struct Elements
{
    unsigned char* header{};
    int stride{};
    Array array() const
    {
        Array result;
        std::memcpy(&result, header, sizeof(result));
        if (result.count < 0 || result.count > 4096 || result.count > result.capacity || (result.count && !result.data))
            throw std::runtime_error("Invalid array");
        return result;
    }
    template<class T> T at(const Array& array, int index, int offset = 0) const
    {
        T value;
        std::memcpy(&value, array.data + static_cast<std::size_t>(index) * stride + offset, sizeof(T));
        return value;
    }
};

struct Value
{
    FProperty* field;
    unsigned char* data;
    void check(const wchar_t* kind, int size) const
    {
        if (field->GetClass().GetFName() != FName(kind, find_name, nullptr) || field->GetElementSize() != size)
            throw std::runtime_error("Reflected value type or size changed");
    }
    template<class T> Typed<T> as(const wchar_t* kind) const
    {
        check(kind, sizeof(T));
        return {data};
    }
    template<class T> T read(const wchar_t* kind) const { return as<T>(kind).read(); }
    template<class T> void write(const wchar_t* kind, T value) const { as<T>(kind).write(value); }
    Flag flag() const
    {
        if (field->GetClass().GetFName() != FName(L"BoolProperty", find_name, nullptr)) throw std::runtime_error("Expected a boolean");
        return {static_cast<FBoolProperty*>(field), data};
    }
    bool boolean() const { return flag().read(); }
    Value member(const wchar_t* name) const
    {
        if (field->GetClass().GetFName() != FName(L"StructProperty", find_name, nullptr)) throw std::runtime_error("Expected a struct");
        auto child = property(static_cast<FStructProperty*>(field)->GetStruct().value, name);
        return {child, data + child->GetOffset_ForInternal()};
    }
    void text(const std::wstring& value, UObject* owner = nullptr) const
    {
        auto end = field->ImportText_Direct(value.c_str(), data, owner, 0, nullptr);
        if (!end || *end) throw std::runtime_error("Engine rejected a reflected value");
    }
    Elements elements(const wchar_t* kind, int size) const
    {
        if (field->GetClass().GetFName() != FName(L"ArrayProperty", find_name, nullptr)) throw std::runtime_error("Expected an array");
        auto element = static_cast<FArrayProperty*>(field)->GetInner();
        if (element->GetClass().GetFName() != FName(kind, find_name, nullptr) || element->GetElementSize() != size)
            throw std::runtime_error("Array element type or size changed");
        return {data, size};
    }
};

inline Value member(UObject* object, const wchar_t* name)
{
    if (!object) throw std::runtime_error("Required game object is absent");
    auto field = property(object->GetClassPrivate(), name);
    return {field, reinterpret_cast<unsigned char*>(object) + field->GetOffset_ForInternal()};
}

class Call
{
    UObject* owner;
    UFunction* function;
    FWeakObjectPtr function_reference;
    unsigned char* parameters;
public:
    Call(UObject* object, const wchar_t* name) : Call(permanent, object, name) { function_reference = weak(function); }
    Call(Permanent, UObject* object, const wchar_t* name) : owner(object)
    {
        if (!owner || !(function = owner->GetFunctionByNameInChain(name)))
            throw std::runtime_error("Required function is absent: " + narrow(name));
        auto size = function->GetParmsSize();
        for (auto field = function->GetChildProperties(); field; field = field->Next())
        {
            auto p = static_cast<FProperty*>(field);
            if (!(static_cast<uint64_t>(p->GetPropertyFlags()) & 0x80)) continue;
            if (p->GetOffset_ForInternal() < 0 || p->GetElementSize() < 1 ||
                p->GetElementSize() > size - p->GetOffset_ForInternal() || p->GetMinAlignment() > 16)
                throw std::runtime_error("Invalid function parameter layout");
        }
        parameters = static_cast<unsigned char*>(FMemory::Malloc(std::max<unsigned short>(size, 1), 16));
        if (!parameters) throw std::bad_alloc();
        std::memset(parameters, 0, size);
        FField* current = function->GetChildProperties();
        try
        {
            for (; current; current = current->Next())
            {
                auto p = static_cast<FProperty*>(current);
                if (static_cast<uint64_t>(p->GetPropertyFlags()) & 0x80)
                    p->InitializeValue(parameters + p->GetOffset_ForInternal());
            }
        }
        catch (...)
        {
            for (auto field = function->GetChildProperties(); field != current; field = field->Next())
            {
                auto p = static_cast<FProperty*>(field);
                if (static_cast<uint64_t>(p->GetPropertyFlags()) & 0x80)
                    p->DestroyValue(parameters + p->GetOffset_ForInternal());
            }
            FMemory::Free(parameters);
            throw;
        }
    }
    Call(const Call&) = delete;
    Call& operator=(const Call&) = delete;
    Call(Call&& other) noexcept : owner(other.owner), function(other.function), function_reference(other.function_reference), parameters(other.parameters) { other.parameters = nullptr; }
    Call& operator=(Call&& other) noexcept
    {
        if (this == &other) return *this;
        release();
        owner = other.owner;
        function = other.function;
        function_reference = other.function_reference;
        parameters = other.parameters;
        other.parameters = nullptr;
        return *this;
    }
    ~Call() { release(); }
    void release()
    {
        if (!parameters) return;
        if (function_reference.Get())
            for (auto field = function->GetChildProperties(); field; field = field->Next())
            {
                auto p = static_cast<FProperty*>(field);
                if (static_cast<uint64_t>(p->GetPropertyFlags()) & 0x80)
                    p->DestroyValue(parameters + p->GetOffset_ForInternal());
            }
        FMemory::Free(parameters);
        parameters = nullptr;
    }
    Value operator[](const wchar_t* name)
    {
        auto p = property(function, name);
        if (!(static_cast<uint64_t>(p->GetPropertyFlags()) & 0x80))
            throw std::runtime_error("Requested field is not a function parameter");
        return {p, parameters + p->GetOffset_ForInternal()};
    }
    void run() { owner->ProcessEvent(function, parameters); }
};

template<class T> struct Retained
{
    Call call;
    Typed<T> value;
    Retained(UObject* object, const wchar_t* function, const wchar_t* name, const wchar_t* kind)
        : call(object, function), value(call[name].as<T>(kind)) {}
    Retained(UObject* object, const wchar_t* function, const wchar_t* name, const wchar_t* field, const wchar_t* kind)
        : call(object, function), value(call[name].member(field).as<T>(kind)) {}
    T get() { call.run(); return value.read(); }
    void set(T next) { value.write(next); call.run(); }
};

template<> struct Retained<bool>
{
    Call call;
    Flag value;
    Retained(UObject* object, const wchar_t* function, const wchar_t* name)
        : call(object, function), value(call[name].flag()) {}
    bool get() { call.run(); return value.read(); }
};

template<> struct Retained<std::wstring>
{
    UObject* owner;
    Call call;
    Value value;
    Retained(UObject* object, const wchar_t* function, const wchar_t* name)
        : owner(object), call(object, function), value(call[name]) {}
    void set(const std::wstring& text) { value.text(text, owner); call.run(); }
};

inline UObject* find(const wchar_t* path)
{
    return UObjectGlobals::FindObject(nullptr, nullptr, path, false, nullptr);
}
inline FWeakObjectPtr weak(UObject* object)
{
    Call reference(permanent, find(L"/Script/Engine.Default__KismetSystemLibrary"), L"Conv_ObjectToSoftObjectReference");
    reference[L"Object"].write(L"ObjectProperty", object);
    reference.run();
    FWeakObjectPtr result;
    result.index = object->GetInternalIndex();
    result.serial = FUObjectArray::IndexToObject(result.index)->GetSerialNumber();
    if (!result.serial) throw std::runtime_error("Engine did not allocate an object serial number");
    return result;
}
inline bool live(UObject* object)
{
    constexpr auto excluded = static_cast<EObjectFlags>(16 | 32 | 32768 | 65536);
    return object && !object->HasAnyFlags(excluded) && !object->IsUnreachable();
}
inline void set(UObject* object, const wchar_t* function, const wchar_t* parameter, const std::wstring& value)
{
    Call call(object, function); call[parameter].text(value, object); call.run();
}
inline UObject* attach(UObject* object, const wchar_t* function, UObject* child)
{
    Call call(object, function); call[L"Content"].write(L"ObjectProperty", child); call.run();
    return call[L"ReturnValue"].read<UObject*>(L"ObjectProperty");
}
}
