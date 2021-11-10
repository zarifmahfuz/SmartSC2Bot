#include "Bot.h"


//enum CommandCenterState { BUILD, PREUPGRADE_TRAINSCV, OC, DROPMULE, POSTUPGRADE_TRAINSCV };
void Bot::ChangeCCState(Tag cc) {
    switch (CCStates[cc]) {
        case (CommandCenterState::PREUPGRADE_TRAINSCV): {
            CCStates[cc] = CommandCenterState::OC;
            break;
        }
        // TODO: Change state to DROPMULE and after dropping Mule, change to POSTUPGRADE_TRAINSCV
        case (CommandCenterState::OC): {
            CCStates[cc] = CommandCenterState::POSTUPGRADE_TRAINSCV;
            break;
        }
        default: {
            break;
        }
    }
}

bool Bot::TryBuildCommandCenter(){
    const ObservationInterface *observation = Observation();    
    bool buildCommand = false;
    size_t commandCount = CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER);

    size_t command_cost = Observation()->GetUnitTypeData()[UnitTypeID(UNIT_TYPEID::TERRAN_COMMANDCENTER)].mineral_cost;
    size_t mineral_count = Observation()->GetMinerals();
    // build second command center at supply 19 (currently only builds 1 command center)
    if (mineral_count >= command_cost ){
        buildCommand = true;
        std::cout << "DEBUG: Build second command center" << mineral_count << " " << command_cost << "\n";
    }
    return (buildCommand == true) ? (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER)) : false;
}

bool Bot::TryUpgradeToOC(size_t n) {
    if (n < 1) {
        return false;
    }
    // if the n'th CC has been built
    if ( n <= command_center_tags.size() ) {
        const Unit *unit = Observation()->GetUnit(command_center_tags.at(n-1));

        if ( unit->is_alive ) {
            // upgrade the CC to OC if we have enough resources
            if (Observation()->GetMinerals() >= 150) {
                Actions()->UnitCommand(unit, ABILITY_ID::MORPH_ORBITALCOMMAND);
                std::cout << "DEBUG: Upgrade the " << n << "'th CC to Orbital Command\n";
                return true;
            } else {
                return false;
            } 
        }
    }
    return false;
}



void Bot::CommandCenterHandler() {
    if (command_center_tags.size() < 1) {
        return;
    }

    // build second command center at supply 19
    if (Observation()->GetFoodUsed() >= config.secondCommandCenter && command_center_tags.size()==1) {
                TryBuildCommandCenter();     
    }

    // get the first CC
    const Unit *first_cc_unit = Observation()->GetUnit(command_center_tags.at(0));
    if (CCStates[first_cc_unit->tag] == CommandCenterState::PREUPGRADE_TRAINSCV) {
        // if the first Barracks is ready, upgrade to OC
        if (barracks_tags.size() > 0) {
            ChangeCCState(first_cc_unit->tag);
        } else {
            if (first_cc_unit->orders.size() == 0) {
                Actions()->UnitCommand(first_cc_unit, ABILITY_ID::TRAIN_SCV);
            }
        }
    } else if (CCStates[first_cc_unit->tag] == CommandCenterState::OC) {
        if (CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) < 1) {
            TryUpgradeToOC(1);
        } else {
            ChangeCCState(first_cc_unit->tag);
        }
    } 
    else if (CCStates[first_cc_unit->tag] == CommandCenterState::POSTUPGRADE_TRAINSCV) {
        if (first_cc_unit->orders.size() == 0) {
            Actions()->UnitCommand(first_cc_unit, ABILITY_ID::TRAIN_SCV);
        }
    }
}