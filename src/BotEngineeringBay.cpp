#include "Bot.h"

void Bot::EBayHandler() {

    std::string first_ebay = "first";

    if (first_e_bay_state == EBAYBUILD) {
        // if a E-Bay is not already being built
        if (CountUnitType(UNIT_TYPEID::TERRAN_ENGINEERINGBAY) == 0) {
            TryBuildEBay(first_ebay);
        } else {
            ChangeFirstEbayState();
        }

    } else if (first_e_bay_state == INFANTRYWEAPONSUPGRADELEVEL1) {
        TryInfantryWeaponsUpgrade(1); // use the first E-Bay
    }
}

void Bot::ChangeFirstEbayState() {
    switch (first_e_bay_state) {
        case (EBayState::EBAYBUILD): {
            first_e_bay_state = INFANTRYWEAPONSUPGRADELEVEL1;
        }

        default: {
            break;
        }

    }

}

bool Bot::TryBuildEBay(std::string &ebays_) {

    size_t supply_count = Observation()->GetFoodUsed();
    int required_supply_count = config.engineeringBayFirst;
    if (supply_count >= required_supply_count && canAffordUnit(UNIT_TYPEID::TERRAN_ENGINEERINGBAY)) {
        std::cout << "DEBUG: Build " << ebays_ << " E-Bay\n";
        // order an SCV to build barracks
        return TryBuildStructure(ABILITY_ID::BUILD_ENGINEERINGBAY, UNIT_TYPEID::TERRAN_SCV, true);
    }

    return false;
}

bool Bot::TryInfantryWeaponsUpgrade(size_t n) {
    if (n < 1) { return false; }

    // if the n'th E-Bay has been built
    if (n <= e_bay_tags.size()) {
        const Unit *unit = Observation()->GetUnit(e_bay_tags.at(n - 1));

        if (unit == nullptr) { return false; }

        Actions()->UnitCommand(unit, ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1);

        return true;
    }
    return false;
}


