#ifndef BASICSC2BOT_BOTCONFIG_H
#define BASICSC2BOT_BOTCONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>
#include <unordered_map>

class BotConfig {
public:
    static BotConfig from_file(const std::string &filename);
    const int secondCommandCenter;
    const int engineeringBayMinSupply;
    const int engineeringBayMaxSupply;
    const int engineeringBayFirst;
    const int maxMedivacs;
    const int attackTriggerTimeSeconds;

    // supply levels to build each barracks at
    const std::vector<int> supplyToBuildBarracksAt;

    // a dictionary that maps the n'th Supply Depot to it's required supply level
    std::unordered_map<std::string, int> supply_depot;

    // a dictionary that maps the n'th Refinery to it's required supply level
    std::unordered_map<std::string, int> refinery;

    // defines the maximum number of scouts that will be active during the game at a time
    const int maxSimulScouts;

private:
    explicit BotConfig(const YAML::Node&);
};

#endif //BASICSC2BOT_BOTCONFIG_H
