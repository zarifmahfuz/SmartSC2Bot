//
//  Tutorial.cpp
//  BasicSc2Bot
//
//  Created by Zarif Mahfuz on 2021-10-16.
//

#include <sc2api/sc2_api.h>
#include "sc2api/sc2_unit_filters.h"
#include <iostream>

using namespace sc2;

class Bot : public Agent
{
public:
    virtual void OnGameStart() final {
        std::cout << "Hello World!" << std::endl;
    }

    virtual void OnStep() final {
        TryBuildSupplyDepot();
        TryBuildBarracks();
    }

    size_t CountUnitType(UNIT_TYPEID unit_type) {
        return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
    }
    
    // this will get called each time a unit has no orders in the current step
    // both buildings and army are considered as Unit objects in SC2
    virtual void OnUnitIdle(const Unit *unit) final {
        switch(unit->unit_type.ToType()) {
            // build/train? a Terran Space Construction Vehicle (SCV)
            case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
                break;
            }
            case UNIT_TYPEID::TERRAN_SCV: {
                // if an SCV is idle, tell it mine minerals
                const Unit *mineral_target = FindNearestMineralPatch(unit->pos);
                if (!mineral_target) {
                    break;
                }
                // the ability type SMART is equivalent to a right click when you have a unit selected
                Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
                break;
            }
            case UNIT_TYPEID::TERRAN_BARRACKS: {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
                break;
            }
            case UNIT_TYPEID::TERRAN_MARINE: {
                const GameInfo& game_info = Observation()->GetGameInfo();
                // command the unit to attack the first enemy location
                Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
                break;
            }
            default: {
                break;
            }
        }
    }

    const Unit *FindNearestMineralPatch(const Point2D &start) {
        Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
        float distance = std::numeric_limits<float>::max();
        const Unit* target = nullptr;
        for (const auto &u: units) {
            if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
                float d = DistanceSquared2D(u->pos, start);
                if (d < distance) {
                    distance = d;
                    target = u;
                }
            }
        }
        return target;
    }
    

    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV) {
        const ObservationInterface* observation = Observation();

        // If a unit already is building a supply structure of this type, do nothing.
        // Also get an scv to build the structure.
        const Unit* unit_to_build = nullptr;
        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto& unit : units) {
            for (const auto& order : unit->orders) {
                if (order.ability_id == ability_type_for_structure) {
                    return false;
                }
            }

            if (unit->unit_type == unit_type) {
                unit_to_build = unit;
            }
        }

        float rx = GetRandomScalar();
        float ry = GetRandomScalar();

        // issue a command to the selected unit
        Actions()->UnitCommand(unit_to_build,
            ability_type_for_structure,
            Point2D(unit_to_build->pos.x + rx * 15.0f, unit_to_build->pos.y + ry * 15.0f));

        return true;
    }

    bool TryBuildSupplyDepot() {
        const ObservationInterface *observation = Observation();

        // if we are not supply capped, don't build a supply depot
        if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2) {
            return false;
        }
        // try and build a supply depot. Find a random SCV and give it the order.
        return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
    }

    bool TryBuildBarracks() {
        const ObservationInterface *observation = Observation();

        if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
            // will not build barracks until we have at least one supply depot
            return false;
        }

        // will only build one barracks
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
            return false;
        }

        return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
    }

};

int main(int argc, char* argv[]) {
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);

    Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateComputer(Race::Zerg)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame("/Applications/StarCraft II/Maps/Ladder2017Season1/BelShirVestigeLE.SC2Map");
    while (coordinator.Update()) {
    }

    return 0;
}
