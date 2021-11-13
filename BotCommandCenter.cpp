#include "Bot.h"

void Bot::ChangeFirstCCState() {
    switch (first_cc_state) {
        case (CommandCenterState::PREUPGRADE_TRAINSCV): {
            first_cc_state = CommandCenterState::OC;
            break;
        }
            // TODO: Change state to DROPMULE and after dropping Mule, change to POSTUPGRADE_TRAINSCV
        case (CommandCenterState::OC): {
            first_cc_state = CommandCenterState::POSTUPGRADE_TRAINSCV;
            first_cc_drop_mules = true;
            break;
        }
        default: {
            break;
        }
    }
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
    // get the first CC
    const Unit *first_cc_unit = Observation()->GetUnit(command_center_tags.at(0));
    if (first_cc_state == CommandCenterState::PREUPGRADE_TRAINSCV) {
        // if the first Barracks is ready, upgrade to OC
        if (barracks_tags.size() > 0) {
            ChangeFirstCCState();
        } else {
            if (first_cc_unit->orders.size() == 0) {
                Actions()->UnitCommand(first_cc_unit, ABILITY_ID::TRAIN_SCV);
            }
        }
    } else if (first_cc_state == CommandCenterState::OC) {
        if (CountUnitType(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) < 1) {
            TryUpgradeToOC(1);
        } else {
            ChangeFirstCCState();
        }
    } else if (first_cc_state == CommandCenterState::POSTUPGRADE_TRAINSCV) {
        if (first_cc_unit->orders.size() == 0) {
            Actions()->UnitCommand(first_cc_unit, ABILITY_ID::TRAIN_SCV);
        }

        // drop 1 Mule only when there are 0 mules
        if (CountUnitType(UNIT_TYPEID::TERRAN_MULE) == 0 && first_cc_drop_mules) {
            const Unit *target = FindNearestRequestedUnit(first_cc_unit->pos, Unit::Alliance::Neutral,
                                                          UNIT_TYPEID::NEUTRAL_MINERALFIELD); // find the closet mineral field for the Mule to drop to
            Actions()->UnitCommand(first_cc_unit, ABILITY_ID::EFFECT_CALLDOWNMULE, target); // drop mule

        }


    }
}