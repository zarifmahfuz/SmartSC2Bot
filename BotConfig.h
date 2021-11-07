#ifndef BASICSC2BOT_BOTCONFIG_H
#define BASICSC2BOT_BOTCONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

class BotConfig {
public:
    static BotConfig from_file(const std::string &filename);

    const int test_value;
    const int firstSupplyDepot;
    const int secondSupplyDepot;
    const int firstBarracks;
    const int firstRefinery;
    const int secondCommandCenter;
    const int firstScout;
    const int engineeringBayMinSupply;
    const int engineeringBayMaxSupply;
    const int maxMedivacs;

private:
    explicit BotConfig(const YAML::Node&);
};

#endif //BASICSC2BOT_BOTCONFIG_H
