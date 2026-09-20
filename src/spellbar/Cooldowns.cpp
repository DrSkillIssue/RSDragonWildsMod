#include "SpellBar.hpp"
#include "../Log.hpp"
#include <algorithm>
#include <cmath>

namespace Mods
{
bool Cooldowns::poll(Bar& bar, const Settings::SpellBar& config, const Frame& frame)
{
    const auto now = frame.now;
    bool dirty = false;
    if (now < m_timers.poll_until && now >= m_timers.next_poll)
    {
        m_timers.next_poll = now + 250;
        auto pawn = frame.player->pawn(frame.controller);
        auto component = pawn && m_map.stride ? object_at(pawn, m_map.magic) : nullptr;
        dirty = pawn && pawn->GetClassPrivate() != m_map.pawn_class;
        if (component)
        {
            auto data = reinterpret_cast<unsigned char*>(component) + m_map.map;
            Array elements;
            std::memcpy(&elements, data, sizeof(elements));
            int bit_count;
            std::memcpy(&bit_count, data + 40, sizeof(bit_count));
            std::uint32_t* heap_bits;
            std::memcpy(&heap_bits, data + 32, sizeof(heap_bits));
            auto bits = heap_bits ? heap_bits : reinterpret_cast<std::uint32_t*>(data + 16);
            if (elements.count >= 0 && elements.count <= 4096 && elements.count <= elements.capacity && bit_count == elements.count && (elements.count == 0 || elements.data))
                for (int e = 0; e < elements.count; ++e)
                {
                    if (!(bits[e / 32] & (1u << (e % 32)))) continue;
                    auto pair = elements.data + static_cast<std::size_t>(e) * m_map.stride;
                    UObject* key;
                    std::memcpy(&key, pair, sizeof(key));
                    for (int i = 0; i < static_cast<int>(config.slots.size()); ++i)
                    {
                        if (key != bar.slot(i).asset()) continue;
                        float value;
                        std::memcpy(&value, pair + m_map.value, sizeof(value));
                        if (value == m_last_cast[i]) continue;
                        m_last_cast[i] = value;
                        m_cast_seen[i] = true;
                        dirty = true;
                    }
                }
        }
    }
    return dirty || (m_timers.next_update && now >= m_timers.next_update);
}

void Cooldowns::update(Bar& bar, const Settings::SpellBar& config, const Frame& frame)
{
    auto pawn = frame.player->pawn(frame.controller);
    if (pawn && pawn->GetClassPrivate() != m_map.pawn_class) resolve_map(pawn, frame.now);
    for (int i = 0; i < static_cast<int>(config.slots.size()); ++i)
    {
        if (!m_cast_seen[i]) continue;
        m_cast_seen[i] = false;
        start(bar, i, frame);
    }
    advance(bar, config, frame.now);
}

void Cooldowns::resolve_map(UObject* pawn, std::uint64_t now)
{
    auto magic = member(pawn, L"PlayerUtilityMagicComponent");
    auto component = magic.read<UObject*>(L"ObjectProperty");
    if (!component) return;
    auto map = member(component, L"LastCastTimeBySpellData");
    if (map.field->GetClass().GetFName() != FName(L"MapProperty", find_name, nullptr)) throw std::runtime_error("Cast map type changed");
    auto property = static_cast<FMapProperty*>(map.field);
    const auto& layout = property->GetMapLayout();
    if (property->GetKeyProp()->GetClass().GetFName() != FName(L"ObjectProperty", find_name, nullptr) || property->GetKeyProp()->GetElementSize() != sizeof(UObject*) ||
        property->GetValueProp()->GetClass().GetFName() != FName(L"FloatProperty", find_name, nullptr) || property->GetValueProp()->GetElementSize() != sizeof(float) ||
        layout.ValueOffset < static_cast<int>(sizeof(UObject*)) || layout.SetLayout.SparseArrayLayout.Size < layout.ValueOffset + static_cast<int>(sizeof(float)))
        throw std::runtime_error("Cast map layout changed");
    m_map = { pawn->GetClassPrivate(), magic.field->GetOffset_ForInternal(), map.field->GetOffset_ForInternal(), layout.ValueOffset, layout.SetLayout.SparseArrayLayout.Size };
    m_last_cast.fill(0);
    m_timers.next_poll = 0;
    m_timers.poll_until = std::max(m_timers.poll_until, now + 250);
}

void Cooldowns::start(Bar& bar, int index, const Frame& frame)
{
    auto& cell = bar.slot(index);
    if (!live(cell.asset())) return;
    const float length = frame.player->spells().cooldown(cell.asset());
    if (!(length > 0)) return;
    if (!m_clock)
    {
        m_clock.emplace(find(L"/Script/Engine.Default__GameplayStatics"), L"GetTimeSeconds", L"ReturnValue", L"DoubleProperty");
        m_clock->call[L"WorldContextObject"].write(L"ObjectProperty", frame.controller);
    }
    const auto time = m_clock->get();
    DW_LOG_DEBUG("Cast seen for slot {}: map value {:.2f}, duration {:.2f}, clock {:.2f}", index + 1, m_last_cast[index], length, time);
    const auto finish = static_cast<double>(m_last_cast[index]) + length;
    if (finish <= time) return;
    m_running[index] = true;
    m_end[index] = finish;
    m_duration[index] = length;
    cell.show_cooldown(true);
    m_timers.next_update = frame.now;
}

void Cooldowns::advance(Bar& bar, const Settings::SpellBar& config, std::uint64_t now)
{
    if (!m_timers.next_update || now < m_timers.next_update) return;
    DW_ASSERT(m_clock);
    const auto time = m_clock->get();
    bool any = false;
    for (int i = 0; i < static_cast<int>(config.slots.size()); ++i)
    {
        if (!m_running[i]) continue;
        const auto remaining = m_end[i] - time;
        if (remaining <= 0)
        {
            m_running[i] = false;
            bar.slot(i).show_cooldown(false);
            continue;
        }
        any = true;
        bar.slot(i).set_cooldown(std::min(1.0, remaining / m_duration[i]), static_cast<int>(std::ceil(remaining)));
    }
    m_timers.next_update = any ? now + 50 : 0;
}
}
