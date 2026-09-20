#include "Containers.hpp"
#include "../Log.hpp"

namespace Mods
{
void Containers::configure(const Settings::Containers& incoming)
{
    if (m_config == incoming) return;
    stop();
    m_config = incoming;
    if (!incoming.enabled) return;
    Call load(find(L"/Script/Engine.Default__KismetSystemLibrary"), L"LoadAsset_Blocking");
    for (const auto& item : incoming.items)
    {
        for (const auto& container : known_containers)
            if (item.name == container.name) load[L"Asset"].text(container.asset);
        load.run();
        auto asset = load[L"ReturnValue"].read<UObject*>(L"ObjectProperty");
        if (!asset)
        {
            Log::write("Container item is not available: {}", item.name);
            continue;
        }
        asset->SetRootSet();
        m_assets.push_back(asset);
        member(asset, L"Capacity").write<int>(L"IntProperty", item.capacity);
        member(asset, L"AmountDischargedPerUse").write<int>(L"IntProperty", item.per_use);
        DW_LOG_DEBUG("Container tweak applied: {} capacity {}, per use {}.", item.name, item.capacity, item.per_use);
    }
}

void Containers::stop()
{
    for (auto asset : m_assets) asset->ClearRootSet();
    m_assets.clear();
}
}
