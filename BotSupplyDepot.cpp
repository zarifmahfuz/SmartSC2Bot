#include "Bot.h"

void Bot::ChangeSupplyDepotState() {
    switch (supply_depot_state) {
        case (SupplyDepotState::FIRST): {
            supply_depot_state = SupplyDepotState::SECOND;
            break;
        }
        case (SupplyDepotState::SECOND): {
            supply_depot_state = SupplyDepotState::THIRD;
            break;
        }
        case (SupplyDepotState::THIRD): {
            supply_depot_state = SupplyDepotState::CONT;
            break;
        }
        default: {
            break;
        }
    }
}

bool Bot::TryBuildSupplyDepot(std::string &depot_) {
    int supply_count = Observation()->GetFoodUsed();
    int required_supply_count = config.supply_depot.at(depot_);

    if (supply_count >= required_supply_count) {
        std::cout << "DEBUG: Build Supply Depot \n";
        return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
    }
    return false;
}

void Bot::SupplyDepotHandler() {
    // SUPPLY DEPOT INSTRUCTIONS

    if (supply_depot_state == SupplyDepotState::FIRST) {
        std::string depot = "first";
        // need to build the first supply depot if it is not already being built
        if ( CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1 ) {
            TryBuildSupplyDepot(depot);
        } else {
            ChangeSupplyDepotState();
        }
    }
    else if (supply_depot_state == SupplyDepotState::SECOND) {
        std::string depot = "second";
        // need to build the second supply depot if it is not already being built
        if ( CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 2 ) {
            TryBuildSupplyDepot(depot);
        } else {
            ChangeSupplyDepotState();
        }
    } else if (supply_depot_state == SupplyDepotState::THIRD) {
        std::string depot = "third";
        // need to build the third supply depot if it is not already being built
        if ( CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 3 ) {
            TryBuildSupplyDepot(depot);
        } else {
            ChangeSupplyDepotState();
        }
    } else if (supply_depot_state == SupplyDepotState::CONT) {
        std::string depot = "cont";
        // don't build if the unit is already building
        TryBuildSupplyDepot(depot);
    }
}