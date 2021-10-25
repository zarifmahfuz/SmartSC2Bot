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
        : test_value(node["TestValue"].as<int>() ),
          firstSupplyDepot( node["SupplyDepot"]["first"].as<int>() ),
          secondSupplyDepot( node["SupplyDepot"]["second"].as<int>() ),
          firstBarracks( node["Barracks"]["first"].as<int>() ),
          firstRefinery( node["Refinery"]["first"].as<int>() ),
          secondCommandCenter( node["CommandCenter"]["second"].as<int>() ),
          firstScout( node["SCV"]["scout"]["first"].as<int>() ) {
}