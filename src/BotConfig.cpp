#include <iostream>
#include "BotConfig.h"

BotConfig BotConfig::from_file(const std::string &filename) {
    try {
        return BotConfig(YAML::LoadFile(filename));
    } catch (const YAML::Exception &e) {
        std::cerr << "Failed to load bot config file" << std::endl;
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}

BotConfig::BotConfig(const YAML::Node &node)
        : engineeringBayFirst(node["EngineeringBay"]["first"].as<int>()),
          maxSimulScouts(node["CommandCenter"]["maxSimulScouts"].as<int>()),
          supplyToBuildBarracksAt(node["Barracks"]["supplyToBuildAt"].as<std::vector<int>>()),
          attackTriggerTimeSeconds(node["AttackTrigger"]["timeSeconds"].as<int>()),
          attackTriggerArmyUnits(node["AttackTrigger"]["armyUnits"].as<int>()),
          defendRadius(node["Defend"]["radius"].as<float>()),
          stimpackMinHealth(node["Stimpack"]["minHealth"].as<float>()) {
    supply_depot.insert(std::make_pair<std::string, int>("first", node["SupplyDepot"]["first"].as<int>()));
    supply_depot.insert(std::make_pair<std::string, int>("second", node["SupplyDepot"]["second"].as<int>()));
    supply_depot.insert(std::make_pair<std::string, int>("third", node["SupplyDepot"]["third"].as<int>()));
    supply_depot.insert(std::make_pair<std::string, int>("cont", node["SupplyDepot"]["cont"].as<int>()));
    refinery.insert(std::make_pair<std::string, int>("first", node["Refinery"]["first"].as<int>()));
}