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
        case (SupplyDepotState::CONT): {
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

    if (supply_count >= required_supply_count && canAffordUnit(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)) {
        // std::cout << "DEBUG: Build " << depot_ << " Supply Depot \n";
        return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
    }
    return false;
}

void Bot::SupplyDepotHandler() {
    // SUPPLY DEPOT INSTRUCTIONS
    int supply_cap = Observation()->GetFoodCap();
    int supply_used = Observation()->GetFoodUsed();
    int supply_avail = supply_cap - supply_used;

    if (supply_depot_state == SupplyDepotState::FIRST) {
        std::string depot = "first";
        // need to build the first supply depot if it is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
            TryBuildSupplyDepot(depot);
        }
    } else if (supply_depot_state == SupplyDepotState::SECOND) {
        std::string depot = "second";
        // need to build the second supply depot if it is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 2) {
            TryBuildSupplyDepot(depot);
        }
    } else if (supply_depot_state == SupplyDepotState::THIRD) {
        std::string depot = "third";
        // need to build the third supply depot if it is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 3) {
            TryBuildSupplyDepot(depot);
        }
    } else if (supply_depot_state == SupplyDepotState::CONT) {
        std::string depot = "cont";
        // don't build if the unit is already building
        if (supply_avail < 5)
            TryBuildSupplyDepot(depot);
    }
}