#include <sc2api/sc2_unit_filters.h>
#include "Bot.h"
#include <iostream>
#include <vector>

Bot::Bot(const BotConfig &config)
        : config(config) {}

void Bot::OnGameStart() {
    std::cout << "Hello World!" << std::endl;
    Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    first_command_center = units[0]->tag; // get the tag of the command center the game starts with
}

void Bot::OnStep() {
    TryBuildSupplyDepot();
    TryBuildBarracks();
    TryBuildRefinery();
    TryBuildCommandCenter();
    TryBuildFactory();
}

size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Bot::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            // train SCVs in the command center
            TryUpgradeCommand();
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
            break;
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            // if an SCV is idle, tell it mine minerals
            const Unit *mineral_target = FindNearestRequestedUnit(unit->pos, Unit::Alliance::Neutral,
                                                                  UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            if (!mineral_target) {
                break;
            }
            // the ability type SMART is equivalent to a right click when you have a unit selected
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR: {
            // start non-stop Marine production
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            std::cout << "DEBUG: Start non-stop Marine production\n";
            break;
        }

        default: {
            break;
        }
    }

    // if the 2nd and 3rd barracks are idle and ready to start producing marines, do it
    if ((unit == secondBarrack && startMarineProductionSecondBarrack) ||
        (unit == thirdBarrack && startMarineProductionThirdBarrack)) {
        // start non-stop Marine production for the 2nd barrack
        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        // this second train marine command is mainly for the 3rd barrack, so it can train 2 marines at the same time
        // for the 2nd barrack this just means that the 2nd marines will be queued
        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        std::cout << "DEBUG: 2nd or 3rd Barrack, Start non-stop Marine production" << std::endl;

    }
}

void Bot::OnUnitCreated(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            // SCOUT
            size_t num_scv = CountUnitType(UNIT_TYPEID::TERRAN_SCV);
            if (num_scv == static_cast<size_t>(config.firstScout)) {
                // send the SCV to scout
                const GameInfo &game_info = Observation()->GetGameInfo();
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
                std::cout << "DEBUG: Sending an SCV to scout\n";
            }

            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            size_t num_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);
            if (num_barracks == 1) {
                // upgrade the first Barracks to a Reactor immediately after it finishes building
                Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REACTOR_BARRACKS);
                std::cout << "DEBUG: Upgrade first Barracks to Reactor\n";
            }

            break;
        }

        case UNIT_TYPEID::TERRAN_MARINE: {
            Actions()->UnitCommand(unit, ABILITY_ID::EFFECT_STIM_MARAUDER);
        }
        default: {
            break;
        }
    }
}

void Bot::OnBuildingConstructionComplete(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_REFINERY: {
            // when a Refinery is first created it already has one worker mining gas, need to assign two more
            CommandSCVs(2, unit);
            std::cout << "DEBUG: Assign workers on Refinery\n";
            break;
        }

        case UNIT_TYPEID::TERRAN_BARRACKS: {
            size_t num_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

            if (num_barracks == 2) {
                // add the techLab addon to the 2nd barrack
                // The Barrack must be idle to do this
                Actions()->UnitCommand(unit, ABILITY_ID::BUILD_TECHLAB_BARRACKS);
                std::cout << "DEBUG: Add Tech Lab to 2nd barrack" << std::endl;
                secondBarrack = unit;

            } else if (num_barracks == 3) {
                // add the Rector addon to the 3rd barrack
                // The Barrack must be idle to do this
                Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REACTOR_BARRACKS);
                std::cout << "DEBUG: Add Reactor to 3rd barrack" << std::endl;
                thirdBarrack = unit;
            }

            break;
        }

        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB: {
            size_t num_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);
            if (num_barracks >= 2 && secondBarrack != nullptr) {
                // After the Tech Lab has been built on the 2nd barrack, research Stimpack
                // this will research stimpack on the 2nd barrack's techlab, for now
                // If there are more techlabs it may not be the 2nd barrack
                Actions()->UnitCommand(unit, ABILITY_ID::RESEARCH_STIMPACK);

                startMarineProductionSecondBarrack = true; // start non-stop marine production after Stimpack is researched
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR: {
            size_t num_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);
            if (num_barracks >= 3 && thirdBarrack != nullptr) {
                // After the Reactor is built on the 3rd barrack start non-stop marine production
                startMarineProductionThirdBarrack = true;
            }
            break;
        }
        default: {
            break;
        }
    }
}

const Unit *Bot::FindNearestRequestedUnit(const Point2D &start, Unit::Alliance alliance, UNIT_TYPEID unit_type) {
    // get all units of the specified alliance
    Units units = Observation()->GetUnits(alliance);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;

    // iterate over all the units and find the closest unit matching the unit type
    for (const auto &u: units) {
        if (u->unit_type == unit_type) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

bool Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type) {
    const ObservationInterface *observation = Observation();

    // get a unit to build the structure
    const Unit *builder_unit = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    for (const auto &unit: units) {
        for (const auto &order: unit->orders) {
            // if a unit already is building a supply structure of this type, do nothing.
            if (order.ability_id == ability_type_for_structure) {
                return false;
            } else {
                if (order.ability_id != ABILITY_ID::ATTACK_ATTACK) {
                    // don't use a unit that's attacking to build a supply depot
                    builder_unit = unit;
                }
            }
        }
    }

    float rx = GetRandomScalar();
    float ry = GetRandomScalar();

    // issue a command to the selected unit
    Actions()->UnitCommand(builder_unit,
                           ability_type_for_structure,
                           Point2D(builder_unit->pos.x + rx * 15.0f, builder_unit->pos.y + ry * 15.0f));

    return true;
}

bool Bot::TryBuildSupplyDepot() {
    const ObservationInterface *observation = Observation();
    bool buildSupplyDepot = false;
    size_t supplyDepotCount = CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    size_t foodUsed = observation->GetFoodUsed();

    // if supply depot count is 0 and we are reaching supply cap, build the first supply depot
    if (supplyDepotCount == 0) {
        if (foodUsed >= config.firstSupplyDepot) {
            // build the first supply depot
            buildSupplyDepot = true;
            std::cout << "DEBUG: Build first supply depot\n";
        }
    } else if (supplyDepotCount == 1) {
        if (foodUsed >= config.secondSupplyDepot) {
            // build the second supply depot
            buildSupplyDepot = true;
            std::cout << "DEBUG: Build second supply depot\n";
        }
    }

    if (foodUsed >= config.nonStopSupplyDepot) {
        // build supply depots non-stop
        buildSupplyDepot = true;
        std::cout << "DEBUG: None stop supply depot production" << std::endl;
    }

    return buildSupplyDepot && (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT));
}

bool Bot::TryBuildBarracks() {
    bool buildBarracks = false;
    size_t barracksCount = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

    // will only build one barracks
    if (barracksCount == 0) {
        if (Observation()->GetFoodUsed() >= config.firstBarracks) {
            // if supply is above a certain level, build the first barracks
            buildBarracks = true;
            std::cout << "DEBUG: Build first barracks\n";
        }
    }

    // build the 2nd barrack
    if (barracksCount == 1) {
        if (Observation()->GetFoodUsed() >= config.secondBarracks) {
            // if supply >= 19 build the second barrack
            buildBarracks = true;
            std::cout << "DEBUG: Build 2nd barrack" << std::endl;
        }
    }

    // build the 3rd barrack
    if (barracksCount == 2) {
        if (Observation()->GetFoodUsed() >= config.thirdBarracks) {
            // if supply >= 23 build the second barrack
            buildBarracks = true;
            std::cout << "DEBUG: Build 3rd barrack" << std::endl;
        }
    }


    // order an SCV to build barracks
    return (buildBarracks == true) ? (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS)) : false;
}

bool Bot::TryBuildRefinery() {
    bool buildRefinery = false;
    size_t refineryCount = CountUnitType(UNIT_TYPEID::TERRAN_REFINERY);

    if (refineryCount == 0) {
        if (Observation()->GetFoodUsed() >= config.firstRefinery) {
            buildRefinery = true;
            std::cout << "DEBUG: Build first refinery\n";
        }
    } else if (refineryCount == 1) {
        if (Observation()->GetFoodUsed() >= config.secondRefinery) {
            buildRefinery = true;
            std::cout << "DEBUG: Build 2nd refinery" << std::endl;
        }
    }

    if (buildRefinery) {
        const ObservationInterface *observation = Observation();

        // get an SCV to build the structure
        const Unit *builder_unit = nullptr;
        Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
        for (const auto &unit: units) {
            for (const auto &order: unit->orders) {
                // if a unit already is building a refinery, do nothing.
                if (order.ability_id == ABILITY_ID::BUILD_REFINERY) {
                    return false;
                }
            }
            builder_unit = unit;
            break;
        }

        if (!builder_unit) { return false; }

        // get the nearest Vespene geyser
        const Unit *vespene_geyser = FindNearestRequestedUnit(builder_unit->pos, Unit::Alliance::Neutral,
                                                              UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);

        if (!vespene_geyser) { return false; }
        // issue a command to the selected unit
        Actions()->UnitCommand(builder_unit,
                               ABILITY_ID::BUILD_REFINERY,
                               vespene_geyser);
        return true;
    }
    return false;
}

bool Bot::TryBuildCommandCenter() {
    const ObservationInterface *observation = Observation();

    bool buildCommand = false;

    size_t commandCount = CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER);

    // build second command center at supply 19 (currently only builds 1 command center)
    if (commandCount == 1) {
        if (Observation()->GetFoodUsed() >= config.secondCommandCenter) {
            buildCommand = true;
            std::cout << "DEBUG: Build second command center\n";
        }
    }
    return (buildCommand == true) ? (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER)) : false;
}

bool Bot::TryBuildFactory() {
    const ObservationInterface *observation = Observation();
    bool buildFactory = false;
    size_t factoryCount = CountUnitType(UNIT_TYPEID::TERRAN_FACTORY);

    // if factory count is 0, build the first factory
    if (factoryCount == 0) {
        if (observation->GetFoodUsed() >= config.firstFactory) {
            // build the first factory
            buildFactory = true;
            std::cout << "DEBUG: Build first factory depot\n";
        }
    }

    return buildFactory && (TryBuildStructure(ABILITY_ID::BUILD_FACTORY));
}

bool Bot::TryUpgradeStructure(ABILITY_ID ability_type_for_structure) {

    switch (ability_type_for_structure) {
        case ABILITY_ID::MORPH_ORBITALCOMMAND: {
            // upgrade the first command center
            Actions()->UnitCommand(Observation()->GetUnit(first_command_center), ABILITY_ID::MORPH_ORBITALCOMMAND);
            return true;
        }
    }
    return false;
}

bool Bot::TryUpgradeCommand() {
    bool upgradeCommand = false;

    size_t barracksCount = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

    // upgrade command center when there is 1 barracks
    if (barracksCount == 1) {
        upgradeCommand = true;
    }

    return (upgradeCommand == true) ? (TryUpgradeStructure(ABILITY_ID::MORPH_ORBITALCOMMAND)) : false;
}

void Bot::CommandSCVs(int n, const Unit *target, ABILITY_ID ability) {
    if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) >= n) {
        // gather n SCVs
        std::vector<const Unit *> scv_units;
        Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));

        // find SCVs
        for (const auto &unit: units) {
            bool valid_scv = true;
            for (const auto &order: unit->orders) {
                if (order.target_unit_tag == target->tag) {
                    // this SCV already has the command we are trying to issue
                    valid_scv = false;
                }
            }
            if (valid_scv) {
                scv_units.push_back(unit);
            }
            if (scv_units.size() == n) {
                break;
            }
        }
        // finally, issue the command
        Actions()->UnitCommand(scv_units, ability, target);
    }
}
