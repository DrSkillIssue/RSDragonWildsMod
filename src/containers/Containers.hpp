#pragma once
#include "../Configuration.hpp"
#include "../Reflection.hpp"
#include <vector>

namespace Mods
{
class Containers
{
public:
    void configure(const Settings::Containers& config);
    void stop();

private:
    Settings::Containers m_config;
    std::vector<UObject*> m_assets;
};
}
