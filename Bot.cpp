#include <sc2api/sc2_unit_filters.h>
#include "Bot.h"
#include <iostream>
#include <vector>
#include <algorithm>

Bot::Bot(const BotConfig &config)
        : config(config) {}

void Bot::OnGameStart() {
    std::cout << "Hello World!" << std::endl;
    Units units = Observation()->GetUnits(Unit::Alliance::Self,  IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    first_command_center = units[0]->tag; // get the tag of the command center the game starts with
}

void Bot::OnStep() {
    TryBuildSupplyDepot();
    TryBuildBarracks();
    TryBuildRefinery();
    TryBuildCommandCenter();

    // attach Reactor to the first Barracks
    TryBuildBarracksReactor(1);

    if (marine_prod_first_barracks) {
        TryStartMarineProd(1);
    }

    TryBuildEngineeringBay();
    TryResearchInfantryWeapons();
    TryBuildMissileTurret();
    TryBuildStarport();
    TryResearchCombatShield();
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
            const Unit *mineral_target = FindNearestRequestedUnit(unit->pos, Unit::Alliance::Neutral, UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            if (!mineral_target) {
                break;
            }
            // the ability type SMART is equivalent to a right click when you have a unit selected
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            // do not add the tag if it is already present
            auto p = std::find( begin(barracks_tags), end(barracks_tags), unit->tag);
            if (p == end(barracks_tags)) {
                // add the barracks tag to barracks_tags
                barracks_tags.push_back(unit->tag);
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR: {
            // should start Marine production on the first Barracks
            marine_prod_first_barracks = true;
            std::cout << "DEBUG: Reactor complete. Start non-stop Marine production\n";
            break;
        }

        default: {
            break;
        }
    }
}

void Bot::OnUnitCreated(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            // SCOUT
            size_t num_scv = CountUnitType(UNIT_TYPEID::TERRAN_SCV);
            if ( num_scv == static_cast<size_t>(config.firstScout) ) {
                // send the SCV to scout
                const GameInfo& game_info = Observation()->GetGameInfo();
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
                std::cout << "DEBUG: Sending an SCV to scout\n";
            }
            break;
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
    for (const auto &unit : units) {
        for (const auto &order : unit->orders) {
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

    // if supply depot count is 0 and we are reaching supply cap, build the first supply depot
    if (supplyDepotCount == 0) {
        if (observation->GetFoodUsed() >= config.firstSupplyDepot) {
            // build the first supply depot
            buildSupplyDepot = true;
            std::cout << "DEBUG: Build first supply depot\n";
        }
    } else if (supplyDepotCount == 1) {
        if (observation->GetFoodUsed() >= config.secondSupplyDepot) {
            // build the second supply depot
            buildSupplyDepot = true;
            std::cout << "DEBUG: Build second supply depot\n";
        }
    }

    return (buildSupplyDepot == true) ? (TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT)) : false;
}

bool Bot::TryBuildBarracks() {
    bool buildBarracks = false;
    size_t barracksCount = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

    // will only build one barracks
    if (barracksCount == 0) {
        if ( Observation()->GetFoodUsed() >= config.firstBarracks ) {
            // if supply is above a certain level, build the first barracks
            buildBarracks = true;
            std::cout << "DEBUG: Build first barracks\n";
        }
    }

    // order an SCV to build barracks
    return (buildBarracks == true) ? (TryBuildStructure(ABILITY_ID::BUILD_BARRACKS)) : false;
}

bool Bot::TryBuildRefinery() {
    bool buildRefinery = false;
    size_t refineryCount = CountUnitType(UNIT_TYPEID::TERRAN_REFINERY);

    if (refineryCount == 0) {
        if ( Observation()->GetFoodUsed() >= config.firstRefinery ) {
            buildRefinery = true;
            std::cout << "DEBUG: Build first refinery\n";
        }
    }

    if (buildRefinery) {
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

        if (!builder_unit) { return false; }

        // get the nearest vespene geyser
        const Unit *vespene_geyser = FindNearestRequestedUnit(builder_unit->pos, Unit::Alliance::Neutral, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER);

        if (!vespene_geyser) { return false; }
        // issue a command to the selected unit
        Actions()->UnitCommand(builder_unit,
                               ABILITY_ID::BUILD_REFINERY,
                               vespene_geyser);
        return true;
    }
    return false;
}

bool Bot::TryBuildCommandCenter(){
    const ObservationInterface *observation = Observation();

    bool buildCommand = false;

    size_t commandCount = CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER);

    // build second command center at supply 19 (currently only builds 1 command center)
    if (commandCount==1){
        if ( Observation()->GetFoodUsed() >= config.secondCommandCenter ) {
                buildCommand = true;
                std::cout << "DEBUG: Build second command center\n";
        }
    }
    return (buildCommand == true) ? (TryBuildStructure(ABILITY_ID::BUILD_COMMANDCENTER)) : false;
}


bool Bot::TryUpgradeStructure(ABILITY_ID ability_type_for_structure){

    switch(ability_type_for_structure){
        case ABILITY_ID::MORPH_ORBITALCOMMAND:{
            // upgrade the first command center
            Actions()->UnitCommand(Observation()->GetUnit(first_command_center), ABILITY_ID::MORPH_ORBITALCOMMAND);
            return true;
        }
    }
    return false;
}
bool Bot::TryUpgradeCommand(){
    bool upgradeCommand = false;

    size_t barracksCount = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);

    // upgrade command center when there is 1 barracks
    if (barracksCount==1){
        upgradeCommand = true;
    }

    return (upgradeCommand == true) ? (TryUpgradeStructure(ABILITY_ID::MORPH_ORBITALCOMMAND)) : false;
}

void Bot::CommandSCVs(int n, const Unit *target, ABILITY_ID ability) {
    if ( CountUnitType(UNIT_TYPEID::TERRAN_SCV) >= n) {
        // gather n SCVs
        std::vector<const Unit *> scv_units;
        Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));

        // find SCVs
        for (const auto& unit : units) {
            bool valid_scv = true;
            for (const auto &order : unit->orders) {
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

bool Bot::TryBuildBarracksReactor(size_t n) {
    if (n < 1) { return false; }
    
    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));

        // when the Barracks has no add ons, it's add on tag is 0
        if ( unit->is_alive && (unit->add_on_tag == 0) ) {
            // attach a Reactor to this Barracks
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REACTOR_BARRACKS);
            std::cout << "DEBUG: Upgrade " << n << "'th Barracks to Reactor\n";
            return true;
        }
    }
    return false;
}

bool Bot::TryStartMarineProd(size_t n) {
    if (n < 1) { return false; }

    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));
        if ( unit->orders.size() == 0 ) {
            // the barracks is currently idle - order marine production
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            std::cout << "DEBUG: Barracks #" << n << " trains Marine\n";
        }
    }
}

bool Bot::TryBuildEngineeringBay() {
    const auto *observation = Observation();

    // Only build one Engineering Bay
    if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY) > 0)
        return false;

    // Only build if in correct supply range
    if (observation->GetFoodUsed() < config.engineeringBayMinSupply
        || observation->GetFoodUsed() > config.engineeringBayMaxSupply)
        return false;

    return TryBuildStructure(sc2::ABILITY_ID::BUILD_ENGINEERINGBAY);
}

const Unit *Bot::GetEngineeringBay() {
    auto engineering_bays = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ENGINEERINGBAY));
    if (engineering_bays.empty())
        return nullptr;
    return engineering_bays[0];
}

bool Bot::TryResearchInfantryWeapons() {
    // Only research if the Engineering Bay exists
    auto *engineering_bay = GetEngineeringBay();
    if (!engineering_bay)
        return false;

    // Return if Engineering Bay is not yet built,
    // the Engineering Bay already has orders,
    // or the upgrade is already complete
    if (engineering_bay->build_progress < 1.0F
        || !engineering_bay->orders.empty()
        || engineering_bay->attack_upgrade_level >= 1)
        return false;

    // Make upgrade command
    Actions()->UnitCommand(engineering_bay, ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1);
    std::cout << "DEBUG: Researching Infantry Weapons Level 1 on Engineering Bay\n";
    return true;
}

bool Bot::TryBuildMissileTurret() {
    // Only build if there is not already a Missile Turret
    if (CountUnitType(UNIT_TYPEID::TERRAN_MISSILETURRET) > 0)
        return false;

    // Only build if the Engineering Bay exists
    auto *engineering_bay = GetEngineeringBay();
    if (!engineering_bay)
        return false;

    return TryBuildStructure(ABILITY_ID::BUILD_MISSILETURRET);
}

bool Bot::TryBuildStarport() {
    const auto *observation = Observation();

    // Only build one Starport
    if (!observation->GetUnits(IsUnits(
            {sc2::UNIT_TYPEID::TERRAN_STARPORT, sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR}
            )).empty())
        return false;

    // Only build once a Factory is 100% built
    const auto factories = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
    return std::any_of(factories.begin(), factories.end(), [this](const auto &factory) {
        return factory->build_progress >= 1.0F && TryBuildStructure(sc2::ABILITY_ID::BUILD_STARPORT);
    });
}

bool Bot::TryResearchCombatShield() {
    const auto *observation = Observation();

    auto tech_labs = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB));
    if (tech_labs.empty())
        return false;

    auto *tech_lab = tech_labs[0];
    Actions()->UnitCommand(tech_lab, ABILITY_ID::RESEARCH_COMBATSHIELD);
    std::cout << "DEBUG: Researching Combat Shield on Barracks' Tech Lab\n";
    return true;
}