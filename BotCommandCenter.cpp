#include "Bot.h"


//enum CommandCenterState { BUILD, PREUPGRADE_TRAINSCV, OC, DROPMULE, POSTUPGRADE_TRAINSCV };
void Bot::ChangeCCState(Tag cc) {
    switch (CCStates[cc]) {
        case (CommandCenterState::BUILDCC):{
            // when a CC is done building, change state to PREUPGRADE_TRAINSCV
            CCStates[cc] = CommandCenterState::PREUPGRADE_TRAINSCV;
            break;
        }
        case (CommandCenterState::PREUPGRADE_TRAINSCV): {
            CCStates[cc] = CommandCenterState::OC;
            break;
        }

        case (CommandCenterState::OC): {
            // make sure CC upgraded successfully
            // if it has, set state to POSTUPGRADE_TRAINSCV
            CCStates[cc] = CommandCenterState::POSTUPGRADE_TRAINSCV;
            first_cc_drop_mules = true;
            
            break;
        }
        default: {
            break;
        }
    }
}

bool Bot::TryBuildCommandCenter(){
    const ObservationInterface *observation = Observation();    
    bool build = false;

    // build second command center at supply 19 (currently only builds 1 command center)
    if (canAffordUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER)){
        build = true;
        std::cout << "DEBUG: Build second command center\n";
    }
    return (build == true) ? (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER)) : false;
}

bool Bot::TryUpgradeToOC(size_t n) {
    if (n < 1) {
        return false;
    }
    // if the n'th CC has been built
    if (n <= command_center_tags.size()) {
        const Unit *unit = Observation()->GetUnit(command_center_tags.at(n - 1));

        if (unit->is_alive) {
            // upgrade the CC to OC if we have enough resources
            if (canAffordUnit(UNIT_TYPEID::TERRAN_ORBITALCOMMAND)) {
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
        CCStates.clear();
        return;
    }

    // second command center doesn't need to be built for now
    // if (Observation()->GetFoodUsed() >= config.secondCommandCenter) {
    //                 TryBuildCommandCenter();     
    // }

    // Using a loop to manage the states of all command centers
    // This makes it easy to add more command centers in the future
    int n =0;
    for (const Tag &tag: command_center_tags){
        n++; // keep track of the CC number
        const Unit *cc_unit = Observation()->GetUnit(tag);
        switch(CCStates[tag]){
            case CommandCenterState::PREUPGRADE_TRAINSCV:{
                // if the first Barracks is ready, upgrade to OC
                if (barracks_tags.size() > 0 && CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) < 1 && n==1) {
                    ChangeCCState(tag);
                } 
                else if (cc_unit->orders.size() == 0) {
                    Actions()->UnitCommand(cc_unit, ABILITY_ID::TRAIN_SCV);
                }
                break;
            }
            case CommandCenterState::OC:{
                // n==1, so this only applies to the first command center
                // using n, it would be easy to change the states of the FIRST,SECOND... command centers independently
                if (CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) < 1 && n==1) {
                    TryUpgradeToOC(n);
                } else {
                    ChangeCCState(tag);
                }
                break;
            }
            case CommandCenterState::POSTUPGRADE_TRAINSCV:{
                if (cc_unit->orders.size() == 0) {
                    Actions()->UnitCommand(cc_unit, ABILITY_ID::TRAIN_SCV);
                    // drop 1 Mule only when there are 0 mules
                    if (CountUnitType(UNIT_TYPEID::TERRAN_MULE) == 0 && first_cc_drop_mules) {
                        const Unit *target = FindNearestRequestedUnit(cc_unit->pos, Unit::Alliance::Neutral,
                                                                      UNIT_TYPEID::NEUTRAL_MINERALFIELD); // find the closet mineral field for the Mule to drop to
                        Actions()->UnitCommand(cc_unit, ABILITY_ID::EFFECT_CALLDOWNMULE, target); // drop mule
                    }
                }
            }
            default:
                break;
        }
 
    }
}