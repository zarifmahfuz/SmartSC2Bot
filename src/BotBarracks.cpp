#include <sc2api/sc2_unit_filters.h>

#include "Bot.h"

void Bot::TryBuildingBarracks() {
    const auto *observation = Observation();
    auto num_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);
    auto current_supply = observation->GetFoodUsed();
    auto required_supply = config.supplyToBuildBarracksAt.at(num_barracks);

    if (current_supply >= required_supply && canAffordUnit(UNIT_TYPEID::TERRAN_BARRACKS)) {
        // order an SCV to build barracks
        TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV, true);
    }
}

void Bot::TryBuildingBarracksReactor(const Unit *barracks) {
    assert(barracks->unit_type == UNIT_TYPEID::TERRAN_BARRACKS);
    Actions()->UnitCommand(barracks, ABILITY_ID::BUILD_REACTOR_BARRACKS);
}

void Bot::TryBuildingBarracksTechLab(const Unit *barracks) {
    assert(barracks->unit_type == UNIT_TYPEID::TERRAN_BARRACKS);
    Actions()->UnitCommand(barracks, ABILITY_ID::BUILD_TECHLAB_BARRACKS);
}

void Bot::TryResearchingStimpack(const Unit *tech_lab) {
    assert(tech_lab->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);

    if (canAffordUpgrade(UPGRADE_ID::STIMPACK)) {
        std::cout << "DEBUG: Researching Stimpack at a Barracks Tech Lab\n";
        Actions()->UnitCommand(tech_lab, ABILITY_ID::RESEARCH_STIMPACK);
    }
}

void Bot::TryResearchingCombatShield(const Unit *tech_lab) {
    assert(tech_lab->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);

    if (canAffordUpgrade(UPGRADE_ID::COMBATSHIELD)) {
        std::cout << "DEBUG: Researching Combat Shield at a Barracks Tech Lab\n";
        Actions()->UnitCommand(tech_lab, ABILITY_ID::RESEARCH_COMBATSHIELD);
    }
}

void Bot::TryProducingMarine(const Unit *barracks) {
   assert(barracks->unit_type == UNIT_TYPEID::TERRAN_BARRACKS);
   Actions()->UnitCommand(barracks, ABILITY_ID::TRAIN_MARINE);
}

void Bot::TryProducingMarauder(const Unit *barracks) {
    assert(barracks->unit_type == UNIT_TYPEID::TERRAN_BARRACKS);
    Actions()->UnitCommand(barracks, ABILITY_ID::TRAIN_MARAUDER);
}

void Bot::BarracksHandler() {
    // BARRACKS INSTRUCTIONS
    assert(barracks_tags.size() == barracks_states.size() && barracks_tags.size() == barracks_tech_lab_states.size());
    const auto *observation = Observation();

    // Try to build barracks if we have reached the supply threshold for the next barracks
    size_t number_of_barracks = CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS);
    if (number_of_barracks < config.supplyToBuildBarracksAt.size()
        && observation->GetFoodUsed() >= config.supplyToBuildBarracksAt[number_of_barracks]) {
        TryBuildingBarracks();
    }

    for (size_t i = 0; i < barracks_tags.size(); ++i) {
        const auto *unit = observation->GetUnit(barracks_tags[i]);

        if (unit == nullptr) {
            std::cout << "DEBUG: Barracks i=" << i << " unit is a nullptr\n";
            continue;
        }
        const Unit* add_on = observation->GetUnit(unit->add_on_tag);
        auto &state = barracks_states[i];
        

        if (state == BarracksState::BUILDING) {
            if (unit->IsBuildFinished()) {
                state = i == 0 ? BarracksState::BUILDING_TECH_LAB : BarracksState::BUILDING_REACTOR;
            }
        } else if (state == BarracksState::BUILDING_TECH_LAB) {
            if (add_on != nullptr && add_on->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB) {
                state = BarracksState::PRODUCING_MARINES;
            } else {
                TryBuildingBarracksTechLab(unit);
            }
        } else if (state == BarracksState::BUILDING_REACTOR) {
            if (add_on != nullptr && add_on->unit_type == UNIT_TYPEID::TERRAN_BARRACKSREACTOR) {
                state = BarracksState::PRODUCING_MARINES;
            } else {
                TryBuildingBarracksReactor(unit);
            }
        } else if (state == BarracksState::PRODUCING_MARINES) {
            // do not overload the queue
            if (unit->orders.size() < 2)
                TryProducingMarine(unit);

            if (add_on != nullptr && add_on->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB) {
                state = BarracksState::PRODUCING_MARAUDERS;
            }
        } else if (state == BarracksState::PRODUCING_MARAUDERS) {
            // do not overload the queue
            if (unit->orders.size() < 1)
                TryProducingMarauder(unit);
        }

        if (add_on != nullptr && add_on->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB)
            BarracksTechLabHandler(add_on, barracks_tech_lab_states[i]);
    }
}

void Bot::BarracksTechLabHandler(const Unit *tech_lab, BarracksTechLabState &state) {
    assert(tech_lab->unit_type == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);

    if (state == BarracksTechLabState::NONE) {
        state = BarracksTechLabState::BUILDING;
    } else if (state == BarracksTechLabState::BUILDING && tech_lab->IsBuildFinished()) {
        state = BarracksTechLabState::RESEARCHING_STIMPACK;
    } else if (state == BarracksTechLabState::RESEARCHING_STIMPACK) {
        if (have_stimpack) {
            state = BarracksTechLabState::RESEARCHING_COMBAT_SHIELD;
        } else {
            TryResearchingStimpack(tech_lab);
        }
    } else if (state == BarracksTechLabState::RESEARCHING_COMBAT_SHIELD) {
        if (have_combat_shield) {
            state = BarracksTechLabState::DONE;
        } else {
            TryResearchingCombatShield(tech_lab);
        }
    }
}