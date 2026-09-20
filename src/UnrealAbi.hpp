#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace RC::Unreal
{
class UClass;
class UScriptStruct;
class FProperty;
class UFunction;
class UWorld;
class UObject;
class FOutputDevice;
enum class EObjectFlags : std::uint32_t;
enum class EPropertyFlags : std::uint64_t;
enum class EFindName : int;
class FString;
struct ObjectSearcher;
template<class T> class TObjectPtr { public: T* value; };
class FName
{
    std::uint32_t index, number;
public:
    __declspec(dllimport) FName();
    __declspec(dllimport) FName(const wchar_t*, EFindName, void*);
    __declspec(dllimport) std::wstring ToString();
    bool operator==(const FName& other) const { return index == other.index && number == other.number; }
    bool operator!=(const FName& other) const { return !(*this == other); }
    bool none() const { return index == 0; }
};
class FFieldClassVariant
{
    void* container;
    bool is_object;
public:
    __declspec(dllimport) std::wstring GetName() const;
    __declspec(dllimport) FName GetFName() const;
};
class FField
{
    __declspec(dllimport) FField*& GetNext();
public:
    __declspec(dllimport) FFieldClassVariant GetClass();
    __declspec(dllimport) std::wstring GetName();
    __declspec(dllimport) FName GetFName() const;
    FField* Next() { return GetNext(); }
};
class FProperty : public FField
{
public:
    __declspec(dllimport) int GetOffset_ForInternal() const;
    __declspec(dllimport) int& GetElementSize();
    __declspec(dllimport) int GetMinAlignment() const;
    __declspec(dllimport) EPropertyFlags& GetPropertyFlags();
    __declspec(dllimport) void ExportTextItem_Direct(FString&, const void*, const void*, UObject*, int, UObject*) const;
    __declspec(dllimport) void InitializeValue(void*) const;
    __declspec(dllimport) void DestroyValue(void*) const;
    __declspec(dllimport) void CopyCompleteValue(void*, const void*) const;
    __declspec(dllimport) const wchar_t* ImportText_Direct(const wchar_t*, void*, UObject*, int, FOutputDevice*) const;
};
class FMemory
{
public:
    __declspec(dllimport) static void* Malloc(std::uint64_t, std::uint32_t);
    __declspec(dllimport) static void Free(void*);
};
class UObjectBase
{
public:
    __declspec(dllimport) UClass*& GetClassPrivate();
    __declspec(dllimport) int GetInternalIndex();
};
class UObject : public UObjectBase
{
public:
    __declspec(dllimport) std::wstring GetFullName(UObject*) const;
    __declspec(dllimport) UWorld* GetWorld() const;
    __declspec(dllimport) bool HasAnyFlags(EObjectFlags);
    __declspec(dllimport) bool IsUnreachable();
    __declspec(dllimport) FProperty* GetPropertyByNameInChain(const wchar_t*);
    __declspec(dllimport) std::wstring GetName() const;
    __declspec(dllimport) UFunction* GetFunctionByNameInChain(const wchar_t*);
    __declspec(dllimport) void ProcessEvent(UFunction*, void*);
    __declspec(dllimport) void SetRootSet();
    __declspec(dllimport) void ClearRootSet();
    __declspec(dllimport) bool IsRootSet();
};
class UField : public UObject
{
public:
    __declspec(dllimport) TObjectPtr<UField>& GetNext();
};
class UStruct : public UField
{
public:
    __declspec(dllimport) FField*& GetChildProperties();
    __declspec(dllimport) TObjectPtr<UField>& GetChildren();
    __declspec(dllimport) UStruct*& GetSuperStruct();
};
class UScriptStruct : public UStruct {};
class UClass : public UStruct
{
public:
    __declspec(dllimport) TObjectPtr<UObject>& GetClassDefaultObject();
};
struct FFrame;
class UnrealScriptFunctionCallableContext
{
public:
    UObject* Context;
    FFrame& TheStack;
    void* RESULT_DECL;
};
class UFunction : public UStruct
{
public:
    __declspec(dllimport) unsigned short& GetParmsSize();
    __declspec(dllimport) unsigned short& GetReturnValueOffset();
    __declspec(dllimport) unsigned int& GetFunctionFlags();
    __declspec(dllimport) int RegisterPostHook(const std::function<void(UnrealScriptFunctionCallableContext&, void*)>&, void*);
    __declspec(dllimport) bool UnregisterHook(int);
};
class FNumericProperty : public FProperty {};
class FEnumProperty : public FProperty
{
public:
    __declspec(dllimport) FNumericProperty*& GetUnderlyingProp();
};
class FStructProperty : public FProperty
{
public:
    __declspec(dllimport) TObjectPtr<UScriptStruct>& GetStruct();
};
class FArrayProperty : public FProperty
{
public:
    __declspec(dllimport) FProperty*& GetInner();
};
class FBoolProperty : public FProperty
{
public:
    __declspec(dllimport) bool GetPropertyValue(const void*);
    __declspec(dllimport) void SetPropertyValue(void*, bool);
};
class FString
{
public:
    wchar_t* data;
    int count, capacity;
    __declspec(dllimport) FString();
    __declspec(dllimport) ~FString();
};
struct FWeakObjectPtr
{
    int index = -1, serial = 0;
    __declspec(dllimport) UObject* Get() const;
};
struct FUObjectItem
{
    __declspec(dllimport) int& GetSerialNumber();
};
class FUObjectArray
{
public:
    __declspec(dllimport) static FUObjectItem* IndexToObject(int);
};
struct FStaticConstructObjectParameters
{
    const UClass* type;
    UObject* outer;
    FName name;
    std::uint32_t flags, internal_flags;
    bool copy_transients, assume_archetype;
    UObject* object_template;
    void* instance_graph;
    void* external_package;
    __declspec(dllimport) FStaticConstructObjectParameters(const UClass*, UObject*);
};
struct FScriptMapLayout
{
    int ValueOffset;
    struct { int HashNextIdOffset, HashIndexOffset, Size; struct { int Alignment, Size; } SparseArrayLayout; } SetLayout;
};
class FMapProperty : public FProperty
{
public:
    __declspec(dllimport) FProperty*& GetKeyProp();
    __declspec(dllimport) FProperty*& GetValueProp();
    __declspec(dllimport) FScriptMapLayout& GetMapLayout();
};
namespace UObjectGlobals
{
    __declspec(dllimport) UObject* FindObject(UClass*, UObject*, const wchar_t*, bool, ObjectSearcher*);
    __declspec(dllimport) UObject* FindFirstOf(const wchar_t*);
    __declspec(dllimport) void FindAllOf(const wchar_t*, std::vector<UObject*>&);
    __declspec(dllimport) UObject* StaticConstructObject(const FStaticConstructObjectParameters&);
}
class UEngine;
namespace Hook
{
    struct FCallbackOptions
    {
        bool bOnce = false;
        bool bReadonly = false;
        std::wstring OwnerModName;
        std::wstring HookName;
    };
    template<class ReturnType> class TCallbackIterationData;
    using GlobalCallbackId = std::uint64_t;
    __declspec(dllimport) GlobalCallbackId RegisterEngineTickPostCallback(std::function<void(TCallbackIterationData<void>&, UEngine*, float, bool)>, FCallbackOptions);
    __declspec(dllimport) bool UnregisterCallback(GlobalCallbackId);
}
static_assert(sizeof(Hook::FCallbackOptions) == 8 + 2 * sizeof(std::wstring));
}
namespace RC::Input
{
enum Key : std::uint8_t
{
    LEFT_MOUSE_BUTTON = 0x1,
    RIGHT_MOUSE_BUTTON = 0x2,
    MIDDLE_MOUSE_BUTTON = 0x4,
};
enum ModifierKey : std::uint8_t
{
    SHIFT = 0x10,
    CONTROL = 0x11,
    ALT = 0x12,
    MODIFIER_KEYS_MAX,
};
using ModifierKeyArray = std::array<ModifierKey, MODIFIER_KEYS_MAX>;
struct KeyData
{
    std::uint32_t required_modifier_keys;
    std::function<void()> callback;
    std::uint8_t custom_data;
    void* custom_data2;
    bool requires_modifier_keys;
    bool is_down;
};
struct KeySet
{
    std::unordered_map<Key, std::vector<KeyData>> key_data;
};
__declspec(dllimport) Key string_to_key(const std::wstring& string);
}
namespace RC
{
class CppUserModBase;
struct KeyDownEventData
{
    std::uint8_t custom_data;
    CppUserModBase* mod;
};
class UE4SSProgram
{
public:
    __declspec(dllimport) static UE4SSProgram& get_program();
    __declspec(dllimport) std::wstring get_mods_directory();
    __declspec(dllimport) void get_all_input_events(std::function<void(Input::KeySet&)> callback);
};
}
namespace RC::Ini
{
class Value
{
    std::wstring m_string_value;
    std::int64_t m_int64_value{};
    float m_float_value{};
    bool m_bool_value{};
    std::array<bool, 5> m_valid_types{};
    std::size_t m_num_valid_types{};
    const Value* m_ref{};
public:
    bool is_valid_string() const { return m_valid_types[1]; }
    bool is_valid_int64() const { return m_valid_types[2]; }
    bool is_valid_float() const { return m_valid_types[3]; }
    bool is_valid_bool() const { return m_valid_types[4]; }
    __declspec(dllimport) const std::wstring& get_string_value() const;
    __declspec(dllimport) std::int64_t get_int64_value() const;
    __declspec(dllimport) float get_float_value() const;
    __declspec(dllimport) bool get_bool_value() const;
    __declspec(dllimport) void add_string_value(std::wstring_view data);
    __declspec(dllimport) void add_int64_value(const std::wstring& data, int base);
    __declspec(dllimport) void add_float_value(const std::wstring& data);
    __declspec(dllimport) void add_bool_value(bool data);
    __declspec(dllimport) void set_ref(const Value* reference);
};
struct Section
{
    std::unordered_map<std::wstring, Value> key_value_pairs;
    std::vector<std::wstring> ordered_list;
    std::vector<std::pair<std::wstring, Value>> key_value_array;
    bool is_ordered_list{};
};
class Parser
{
    std::unordered_map<std::wstring, Section> m_sections;
    bool m_parsing_is_complete{};
public:
    __declspec(dllimport) void parse(std::wstring& input);
    const Section* section(const std::wstring& name) const
    {
        const auto found = m_sections.find(name);
        return found == m_sections.end() ? nullptr : &found->second;
    }
};
static_assert(sizeof(Value) == sizeof(std::wstring) + 40);
}
namespace RC::Unreal
{
static_assert(sizeof(FFieldClassVariant) == 16);
static_assert(sizeof(FName) == 8);
static_assert(sizeof(FScriptMapLayout) == 24);
static_assert(sizeof(FString) == 16);
static_assert(sizeof(FWeakObjectPtr) == 8);
static_assert(sizeof(FStaticConstructObjectParameters) == 64);
}
