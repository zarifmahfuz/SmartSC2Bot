#include "Bot.h"

void Bot::ChangeRefineryState() {
    switch (refinery_state) {
        case (RefineryState::REFINERY_FIRST): {
            refinery_state = RefineryState::ASSIGN_WORKERS;
            break;
        }
        case (RefineryState::ASSIGN_WORKERS): {
            refinery_state = RefineryState::REFINERY_IDLE;
            break;
        }
        default: {
            break;
        }
    }
}

bool Bot::TryBuildRefinery(std::string &refinery_) {
    int supply_count = Observation()->GetFoodUsed();
    int required_supply_count = config.refinery.at(refinery_);

    if (supply_count >= required_supply_count && canAffordUnit(UNIT_TYPEID::TERRAN_REFINERY)) {
        const ObservationInterface *observation = Observation();

        // get an SCV to build the structure
        const Unit *builder_unit = nullptr;
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (const auto &unit : units) {
            for (const auto &order : unit->orders) {
                // if a unit already is building a refinery, do nothing.
                if (order.ability_id == ABILITY_ID::BUILD_REFINERY) {
                    return false;
                }
            }
            builder_unit = unit;
            break;
        }

        // no SCVs found
        if (!builder_unit) { return false; }

        // get the nearest vespene geyser
        const Unit *vespene_geyser = FindNearestRequestedUnit(builder_unit->pos, Unit::Alliance::Neutral, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);

        if (!vespene_geyser) { return false; }

        if (canAffordUnit(UNIT_TYPEID::TERRAN_REFINERY)) {
            std::cout << "DEBUG: Build " << refinery_ << " Refinery\n";
            // issue a command to the selected unit
            Actions()->UnitCommand(builder_unit, ABILITY_ID::BUILD_REFINERY, vespene_geyser);
            return true;
        }
    }
    return false;
}

bool Bot::AssignWorkersToRefinery(const Tag &tag, int target_workers) {
    const Unit *refinery_unit = Observation()->GetUnit(tag);
    if (refinery_unit->assigned_harvesters < target_workers) {
        int scvs_required = target_workers - refinery_unit->assigned_harvesters;
        return CommandSCVs(scvs_required, refinery_unit);
    }
    return true;
}

void Bot::RefineryHandler() {
    if (refinery_state == RefineryState::REFINERY_FIRST) {
        // try to build the first refinery
        // do not waste minerals on building the first refinery when the first supply depot has not been built yet
        if (CountUnitType(UNIT_TYPEID::TERRAN_REFINERY) < 1 && CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) > 0) {
            std::string first("first");
            TryBuildRefinery(first);
        }
    } else if (refinery_state == RefineryState::ASSIGN_WORKERS) {
        // changes state when all the refineries have a target amount of workers assigned
        bool change_state = true;
        for (const Tag &tag : refinery_tags) {
            if (AssignWorkersToRefinery(tag, 3) == false) { change_state = false; }
        }
        if (change_state) { ChangeRefineryState(); }
    }
}