#ifndef BASICSC2BOT_BOTCONFIG_H
#define BASICSC2BOT_BOTCONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>
#include <unordered_map>

class BotConfig {
public:
    static BotConfig from_file(const std::string &filename);
    const int secondCommandCenter;
    const int firstScout;
    const int engineeringBayMinSupply;
    const int engineeringBayMaxSupply;
    const int engineeringBayFirst;
    const int maxMedivacs;

    // a dictionary that maps the n'th Supply Depot to it's required supply level
    std::unordered_map<std::string, int> supply_depot;

    // a dictionary that maps the n'th Refinery to it's required supply level
    std::unordered_map<std::string, int> refinery;

    // a dictionary that maps the n'th barracks to it's required supply level
    std::unordered_map<std::string, int> barracks;

private:
    explicit BotConfig(const YAML::Node&);
};

#endif //BASICSC2BOT_BOTCONFIG_H
