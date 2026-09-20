#include "Fake.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace RC::Unreal
{
UClass*& UObjectBase::GetClassPrivate() { return Fake::records.at(this).type; }
int UObjectBase::GetInternalIndex() { return Fake::records.at(this).id; }
FUObjectItem* FUObjectArray::IndexToObject(int index) { return reinterpret_cast<FUObjectItem*>(Fake::objects.at(index)); }
int& FUObjectItem::GetSerialNumber() { return Fake::records.at(this).serial; }
UWorld* UObject::GetWorld() const { return Fake::records.at(this).world; }
std::wstring UObject::GetName() const { return Fake::records.at(this).name; }
TObjectPtr<UObject>& UClass::GetClassDefaultObject() { static TObjectPtr<UObject> none{}; return none; }
bool UObject::HasAnyFlags(EObjectFlags flags) { return Fake::records.at(this).object_flags & static_cast<uint32_t>(flags); }
bool UObject::IsUnreachable() { return !Fake::records.at(this).alive; }
bool UObject::IsRootSet() { return Fake::records.at(this).roots != 0; }

void UObject::SetRootSet()
{
    if (Fake::game.rooting_works) ++Fake::records.at(this).roots;
}

void UObject::ClearRootSet()
{
    auto& record = Fake::records.at(this);
    assert(!record.attached);
    if (record.type == Fake::widget_class)
    {
        assert(record.roots == 1);
        if (!Fake::game.clearing_works) return;
    }
    if (record.roots) --record.roots;
}

UFunction* UObject::GetFunctionByNameInChain(const wchar_t* name)
{
    ++Fake::game.lookups;
    const auto found = Fake::game.functions.find(name);
    if (found == Fake::game.functions.end()) return nullptr;
    return found->second;
}

FProperty* UObject::GetPropertyByNameInChain(const wchar_t* name)
{
    ++Fake::game.lookups;
    for (auto type = reinterpret_cast<UStruct*>(GetClassPrivate()); type; type = Fake::records.at(type).parent)
        for (auto field = Fake::records.at(type).children; field; field = Fake::records.at(field).next)
            if (Fake::records.at(field).name == name) return static_cast<FProperty*>(field);
    return nullptr;
}

FField*& UStruct::GetChildProperties() { return Fake::records.at(this).children; }
UStruct*& UStruct::GetSuperStruct() { return Fake::records.at(this).parent; }
unsigned short& UFunction::GetParmsSize() { return Fake::records.at(this).parameter_size; }

int UFunction::RegisterPostHook(const std::function<void(UnrealScriptFunctionCallableContext&, void*)>& callback, void* data)
{
    auto& hooks = Fake::records.at(this).post_hooks;
    hooks.emplace_back(callback, data);
    return static_cast<int>(hooks.size());
}

bool UFunction::UnregisterHook(int id)
{
    auto& hooks = Fake::records.at(this).post_hooks;
    assert(id >= 1 && id <= static_cast<int>(hooks.size()) && hooks[id - 1].first);
    hooks[id - 1].first = nullptr;
    return true;
}

FField*& FField::GetNext() { return Fake::records.at(this).next; }

std::wstring FField::GetName()
{
    ++Fake::game.lookups;
    return Fake::records.at(this).name;
}

FName FField::GetFName() const
{
    ++Fake::game.lookups;
    return FName(Fake::records.at(this).name.c_str(), find_name, nullptr);
}

FFieldClassVariant FField::GetClass()
{
    FFieldClassVariant variant{};
    const FField* field = this;
    std::memcpy(&variant, &field, sizeof(field));
    return variant;
}

std::wstring FFieldClassVariant::GetName() const
{
    const void* field;
    std::memcpy(&field, this, sizeof(field));
    ++Fake::game.lookups;
    return Fake::records.at(field).kind;
}

FName FFieldClassVariant::GetFName() const
{
    const void* field;
    std::memcpy(&field, this, sizeof(field));
    ++Fake::game.lookups;
    return FName(Fake::records.at(field).kind.c_str(), find_name, nullptr);
}

int FProperty::GetOffset_ForInternal() const { return Fake::records.at(this).offset; }
int& FProperty::GetElementSize() { return Fake::records.at(this).size; }
int FProperty::GetMinAlignment() const { return 1; }
EPropertyFlags& FProperty::GetPropertyFlags() { return Fake::records.at(this).flags; }
void FProperty::InitializeValue(void* data) const { std::memset(data, 0, Fake::records.at(this).size); }
void FProperty::DestroyValue(void*) const { assert(Fake::records.at(this).alive); }
void FProperty::CopyCompleteValue(void* to, const void* from) const { std::memcpy(to, from, Fake::records.at(this).size); }
void FProperty::ExportTextItem_Direct(FString&, const void*, const void*, UObject*, int, UObject*) const {}
FString::FString() : data(nullptr), count(0), capacity(0) {}
FString::~FString() { std::free(data); }

const wchar_t* FProperty::ImportText_Direct(const wchar_t* text, void*, UObject*, int, FOutputDevice*) const
{
    Fake::game.imported_text = text;
    return text + std::wcslen(text);
}

TObjectPtr<UScriptStruct>& FStructProperty::GetStruct() { return reinterpret_cast<TObjectPtr<UScriptStruct>&>(Fake::records.at(this).structure); }
FProperty*& FArrayProperty::GetInner() { return Fake::records.at(this).inner; }
FProperty*& FMapProperty::GetKeyProp() { return Fake::records.at(this).inner; }
FProperty*& FMapProperty::GetValueProp() { return Fake::records.at(this).value_inner; }
FScriptMapLayout& FMapProperty::GetMapLayout() { return Fake::game.map_layout; }
bool FBoolProperty::GetPropertyValue(const void* data) { return *static_cast<const bool*>(data); }
void FBoolProperty::SetPropertyValue(void* data, bool value) { *static_cast<bool*>(data) = value; }

void* FMemory::Malloc(uint64_t size, uint32_t)
{
    ++Fake::game.parameter_allocations;
    return std::malloc(size);
}

void FMemory::Free(void* data)
{
    --Fake::game.parameter_allocations;
    std::free(data);
}

FName::FName() : index(0), number(0) {}

FName::FName(const wchar_t* name, EFindName mode, void*) : index(0), number(0)
{
    for (size_t i = 0; i < Fake::names.size(); ++i)
        if (Fake::names[i] == name) { index = static_cast<uint32_t>(i); return; }
    if (mode != add_name) return;
    Fake::names.emplace_back(name);
    index = static_cast<uint32_t>(Fake::names.size() - 1);
}

std::wstring FName::ToString() { return Fake::names.at(index); }


UObject* FWeakObjectPtr::Get() const
{
    if (index < 0 || !serial) return nullptr;
    auto object = Fake::objects.at(index);
    const auto& record = Fake::records.at(object);
    if (!record.alive || record.serial != serial) return nullptr;
    return object;
}

FStaticConstructObjectParameters::FStaticConstructObjectParameters(const UClass* cls, UObject* owner)
    : type(cls), outer(owner), flags(0), internal_flags(0), copy_transients(false), assume_archetype(false), object_template(nullptr), instance_graph(nullptr), external_package(nullptr) {}
}
