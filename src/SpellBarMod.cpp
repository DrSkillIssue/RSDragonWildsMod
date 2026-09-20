#include "SpellBarMod.hpp"
#include "Log.hpp"
#include <chrono>

using namespace Mods;
using namespace RC::Unreal;

SpellBarMod::SpellBarMod(const std::filesystem::path& directory) : m_configuration(directory)
{
    ModName = L"DragonwildsSpellBar";
    ModVersion = L"0.2.0";
    ModDescription = L"Spell bar and container tweaks";
    ModAuthors = L"DragonwildsMods";
}

SpellBarMod::~SpellBarMod()
{
    Hook::UnregisterCallback(m_tick_callback);
    m_restart.reset();
    release();
    m_containers.stop();
}

void SpellBarMod::on_unreal_init()
{
    try
    {
        m_restart.emplace(L"/Script/Engine.PlayerController:ClientRestart", [this](UObject* controller) { bind(controller); });
    }
    catch (const std::exception& e)
    {
        Log::write("Spell bar mod failed to start: {}", e.what());
        return;
    }
    m_tick_callback = Hook::RegisterEngineTickPostCallback([this](Hook::TCallbackIterationData<void>&, UEngine*, float, bool)
    {
        tick(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    }, { false, true, L"DragonwildsSpellBar", L"Tick" });
    Log::write("Spell bar mod started.");
}

void SpellBarMod::bind(UObject* controller)
{
    if (!controller->GetFunctionByNameInChain(L"IsPlayerReady")) return;
    try
    {
        Call ready(controller, L"IsPlayerReady"); ready.run();
        if (!ready[L"ReturnValue"].boolean()) return;
        release();
        m_player.bind(controller);
        DW_LOG_DEBUG("Player bound after ClientRestart.");
    }
    catch (const std::exception& e)
    {
        Log::write("Spell bar stopped: {}", e.what());
        release();
    }
}

void SpellBarMod::configure()
{
    if (m_configuration.error()[0])
    {
        Log::write("Configuration unchanged: {}", m_configuration.error());
        return;
    }
    configure(m_configuration.settings());
}

void SpellBarMod::configure(const Settings& settings)
{
    Log::debug = settings.debug;
    RC::UE4SSProgram::get_program().get_all_input_events([this](RC::Input::KeySet& events)
    {
        std::erase_if(events.key_data, [this](auto& entry)
        {
            std::erase_if(entry.second, [this](RC::Input::KeyData& data)
            {
                auto event = static_cast<RC::KeyDownEventData*>(data.custom_data2);
                if (data.custom_data != 2 || !event || event->mod != this) return false;
                delete event;
                return true;
            });
            return entry.second.empty();
        });
    });
    const auto& bar = settings.spell_bar;
    if (bar.enabled)
    {
        register_keydown_event(RC::Input::LEFT_MOUSE_BUTTON, {}, [this] { m_presses |= left_click; });
        register_keydown_event(RC::Input::RIGHT_MOUSE_BUTTON, {}, [this] { m_presses |= right_click; });
        register_keydown_event(RC::Input::MIDDLE_MOUSE_BUTTON, {}, [this] { m_presses |= middle_click; });
        for (std::size_t i = 0; i < bar.slots.size(); ++i)
            if (bar.slots[i].code) register_keydown_event(bar.slots[i].code, bar.slots[i].modifiers, [this, bit = 1u << i] { m_presses |= bit; });
    }
    m_presses = 0;
    m_spell_bar.configure(bar);
    m_containers.configure(settings.containers);
    Log::write("Configuration accepted.");
}

void SpellBarMod::tick(std::uint64_t now)
{
    Frame frame;
    frame.now = now;
    try
    {
        if (m_configuration.poll(now)) configure();
        frame.presses = m_presses.exchange(0);
        if (m_player.identity() && !m_player.alive()) release();
        if (m_player.identity()) m_player.describe(frame);
        m_spell_bar.sample(frame);
        if (m_spell_bar.wants_step(frame)) m_spell_bar.step(frame);
        auto edit = m_spell_bar.take_edit();
        if (!edit) return;
        m_configuration.apply(*edit);
        Log::write("Configuration saved from the in-game panel.");
        configure();
    }
    catch (const std::exception& e)
    {
        Log::write("Spell bar stopped: {}", e.what());
        m_spell_bar.configure({ false });
        m_containers.configure({ false });
    }
    catch (...)
    {
        Log::write("Spell bar stopped: Unknown failure");
        m_spell_bar.configure({ false });
        m_containers.configure({ false });
    }
}

void SpellBarMod::release()
{
    m_player.reset();
    m_spell_bar.stop();
}
