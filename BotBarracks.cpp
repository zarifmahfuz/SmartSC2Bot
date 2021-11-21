#include "Bot.h"

void Bot::ChangeFirstBarracksState() {
    switch (first_barracks_state) {
        case (BarracksState::BUILD): {
            first_barracks_state = BarracksState::TECHLAB;
            break;
        }
        case (BarracksState::TECHLAB): {
            first_barracks_state = BarracksState::STIMPACK;
            break;
        }
        case (BarracksState::STIMPACK): {
            first_barracks_state = BarracksState::MARINEPROD;
            break;
        }
        default: {
            break;
        }
    }
}

void Bot::ChangeSecondBarracksState() {
    switch (second_barracks_state) {
        case (BarracksState::BUILD): {
            second_barracks_state = BarracksState::REACTOR;
            break;
        }
        case (BarracksState::REACTOR): {
            second_barracks_state = BarracksState::MARINEPROD;
            break;
        }
        default: {
            break;
        }
    }
}

void Bot::ChangeThirdBarracksState() {
    switch (third_barracks_state) {
        case (BarracksState::BUILD): {
            third_barracks_state = BarracksState::REACTOR;
            break;
        }
        case (BarracksState::REACTOR): {
            third_barracks_state = BarracksState::MARINEPROD;
            break;
        }
        default: {
            break;
        }
    }
}

bool Bot::TryBuildBarracks(std::string &barracks_) {

    int supply_count = Observation()->GetFoodUsed();
    int required_supply_count = config.barracks.at(barracks_);
    if (supply_count >= required_supply_count) {
        // std::cout << "DEBUG: Build " << barracks_ << " barracks\n";
        // order an SCV to build barracks
        return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV, true);
    }
    
    return false;
}

bool Bot::TryBuildBarracksReactor(size_t n) {
    if (n < 1) { return false; }
    
    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));

        // when the Barracks has no add ons, it's add on tag is 0
        if ( unit != nullptr && unit->is_alive && (unit->add_on_tag == 0) ) {
            // attach a Reactor to this Barracks    
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_REACTOR_BARRACKS);
            std::cout << "DEBUG: Attach Reactor on the " << n << "'th Barracks\n";
            return true;
        }
    }
    return false;
}

bool Bot::TryBuildBarracksTechLab(size_t n) {
    if (n < 1) { return false; }
    
    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));

        // when the Barracks has no add ons, it's add on tag is 0
        if ( unit != nullptr && unit->is_alive && (unit->add_on_tag == 0) ) {
            // attach a Tech Lab to this 
            Actions()->UnitCommand(unit, ABILITY_ID::BUILD_TECHLAB_BARRACKS);
            std::cout << "DEBUG: Attach Tech Lab on the " << n << "'th Barracks\n";
            return true;
        }
    }
    return false;
}

bool Bot::TryResearchBarracksStimpack(size_t n) {
    if (n < 1) { return false; }
    
    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));

        // when the Barracks has no add ons, it's add on tag is 0
        if ( unit != nullptr && unit->is_alive && (unit->add_on_tag != 0) ) {
            // research a Stimpack on the Tech Lab of this Barracks - assuming that the add-on is a Tech Lab
            // get the Tech Lab unit
            const Unit *tech_lab_unit = Observation()->GetUnit(unit->add_on_tag);

            // stimpack costs 100 minerals + 100 vespene
            if (Observation()->GetMinerals() >= 100 && Observation()->GetVespene() >= 100) {
                std::cout << "DEBUG: Research Stimpack at the " << n << "'th Barracks\n";
                Actions()->UnitCommand(tech_lab_unit, ABILITY_ID::RESEARCH_STIMPACK);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool Bot::TryStartMarineProd(size_t n, bool has_reactor) {
    if (n < 1) { return false; }

    // if the n'th barracks has been built
    if (n <= barracks_tags.size()) {
        const Unit *unit = Observation()->GetUnit(barracks_tags.at(n-1));
        if ( unit != nullptr && unit->orders.size() == 0 ) {
            // the barracks is currently idle - order marine production
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            if (has_reactor) {
                // can simultaneously produce two units
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
            }
            std::cout << "DEBUG: Barracks #" << n << " trains Marine\n";
            return true;
        }
    }
    return false;
}

void Bot::BarracksHandler() {
    // BARRACKS INSTRUCTIONS

    // state machine for the first Barracks
    std::string first_barracks = "first";
    if (first_barracks_state == BarracksState::BUILD) {
        // if a Barracks is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) < 1) {
            TryBuildBarracks(first_barracks);
        } else {
            ChangeFirstBarracksState();
        }
    }
    else if (first_barracks_state == BarracksState::TECHLAB) {
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB) < 1) {
            TryBuildBarracksTechLab(1);
        } else {
            ChangeFirstBarracksState();
        }
    } else if (first_barracks_state == BarracksState::STIMPACK) {
        if (TryResearchBarracksStimpack(1)) {
            ChangeFirstBarracksState();
        }
    } else if (first_barracks_state == BarracksState::MARINEPROD) {
        TryStartMarineProd(1, false);
    }
    
    // state machine for the second Barracks
    std::string second_barracks = "second";
    if (second_barracks_state == BarracksState::BUILD) {
        // if the second Barracks is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) < 2) {
            TryBuildBarracks(second_barracks);
        } else {
            ChangeSecondBarracksState();
        }
    } else if (second_barracks_state == BarracksState::REACTOR) {
        // the second Barracks is going to be the first Barracks with a Reactor
        // this check makes sure that we don't order a Reactor while it is already under construction
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKSREACTOR) < 1) {
            // attaches Reactor to the second Barracks
            TryBuildBarracksReactor(2);
        } else {
            ChangeSecondBarracksState();
        }
    } else if (second_barracks_state == BarracksState::MARINEPROD) {
        // starts Marine production on the third Barracks
        TryStartMarineProd(2, true);
    }

    // state machine for the third Barracks
    std::string third_barracks = "third";
    if (third_barracks_state == BarracksState::BUILD) {
        // if the third Barracks is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) < 3) {
            TryBuildBarracks(third_barracks);
        } else {
            ChangeThirdBarracksState();
        }
    } else if (third_barracks_state == BarracksState::REACTOR) {
        // the third Barracks is going to be the second Barracks with a Reactor
        // this check makes sure that we don't order a Reactor while it is already under construction
        if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKSREACTOR) < 2) {
            // attaches Reactor to the third Barracks
            TryBuildBarracksReactor(3);
        } else {
            ChangeThirdBarracksState();
        }
    } else if (third_barracks_state == BarracksState::MARINEPROD) {
        // starts Marine production on the third Barracks
        TryStartMarineProd(3, true);
    }
}
