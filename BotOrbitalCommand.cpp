#include "Bot.h"

void Bot::ChangeOCState(Tag oc) {
    switch (OCStates[oc]) {
        case (OrbitalCommandState::POSTUPGRADE_TRAINSCV): {
            OCStates[oc] = OrbitalCommandState::DROPMULE;
            break;
        }
        case (OrbitalCommandState::DROPMULE): {
            OCStates[oc] = OrbitalCommandState::POSTUPGRADE_TRAINSCV;
            break;
        }
        default: {
            break;
        }
    }
}

void Bot::OrbitalCommandHandler() {
    if (orbital_command_tags.size() < 1) {
        OCStates.clear();
        return;
    }
    
    for (const Tag &tag: orbital_command_tags){
        const Unit *oc_unit = Observation()->GetUnit(tag);
        switch(OCStates[tag]){
            case OrbitalCommandState::POSTUPGRADE_TRAINSCV: {
                if (oc_unit->orders.size() == 0){
                    Actions()->UnitCommand(oc_unit, ABILITY_ID::TRAIN_SCV);
                    break;
                }
            }
            // TODO: implement dropping mules
            default:
                break;
        }
    }
}