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
        : firstRefinery( node["Refinery"]["first"].as<int>() ),
          secondCommandCenter( node["CommandCenter"]["second"].as<int>() ),
          firstScout( node["SCV"]["scout"]["first"].as<int>() ),
          engineeringBayMinSupply(node["EngineeringBay"]["minSupply"].as<int>()),
          engineeringBayMaxSupply(node["EngineeringBay"]["maxSupply"].as<int>()),
          maxMedivacs(node["Medivac"]["maxUnits"].as<int>()) {
    supply_depot.insert( std::make_pair<std::string, int>("first", node["SupplyDepot"]["first"].as<int>()) );
    supply_depot.insert( std::make_pair<std::string, int>("second", node["SupplyDepot"]["second"].as<int>()) );
    supply_depot.insert( std::make_pair<std::string, int>("third", node["SupplyDepot"]["third"].as<int>()) );
    supply_depot.insert( std::make_pair<std::string, int>("cont", node["SupplyDepot"]["cont"].as<int>()) );
    barracks.insert( std::make_pair<std::string, int> ("first", node["Barracks"]["first"].as<int>()) );
    barracks.insert( std::make_pair<std::string, int> ("second", node["Barracks"]["second"].as<int>()) );
    barracks.insert( std::make_pair<std::string, int> ("third", node["Barracks"]["third"].as<int>()) );
}