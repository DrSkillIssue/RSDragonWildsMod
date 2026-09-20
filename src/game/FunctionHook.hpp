#pragma once
#include "../Reflection.hpp"
#include <functional>

namespace Mods
{
class FunctionHook
{
public:
    FunctionHook(const wchar_t* path, std::function<void(UObject*)> callback)
        : m_function(static_cast<UFunction*>(find(path))), m_callback(std::move(callback))
    {
        if (!m_function) throw std::runtime_error("Required function is absent: " + narrow(path));
        m_id = m_function->RegisterPostHook([](UnrealScriptFunctionCallableContext& context, void* data) { static_cast<FunctionHook*>(data)->m_callback(context.Context); }, this);
    }
    FunctionHook(const FunctionHook&) = delete;
    FunctionHook& operator=(const FunctionHook&) = delete;
    ~FunctionHook() { m_function->UnregisterHook(m_id); }

private:
    UFunction* m_function;
    std::function<void(UObject*)> m_callback;
    int m_id;
};
}
