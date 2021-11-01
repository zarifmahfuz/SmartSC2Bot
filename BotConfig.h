#ifndef BASICSC2BOT_BOTCONFIG_H
#define BASICSC2BOT_BOTCONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

class BotConfig {
public:
    static BotConfig from_file(const std::string &filename);

    const int test_value;

    // supply depots
    const int firstSupplyDepot;
    const int secondSupplyDepot;

    // barracks
    const int firstBarracks;
    const int secondBarracks;
    const int thirdBarracks;

    // refineries
    const int firstRefinery;
    const int secondRefinery;
    const int secondCommandCenter;

    const int firstScout;

    const int firstFactory;

private:
    explicit BotConfig(const YAML::Node&);
};

#endif //BASICSC2BOT_BOTCONFIG_H
