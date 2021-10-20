#ifndef BASICSC2BOT_BOT_H
#define BASICSC2BOT_BOT_H

#include <sc2api/sc2_api.h>
#include "BotConfig.h"

using namespace sc2;

class Bot : public Agent {
public:
    explicit Bot(const BotConfig &config);

    virtual void OnGameStart() final;
    virtual void OnStep() final;
    // this will get called each time a unit has no orders in the current step
    // both buildings and army are considered as Unit objects in SC2
    virtual void OnUnitIdle(const Unit *unit) final;

    size_t CountUnitType(UNIT_TYPEID unit_type);
    const Unit *FindNearestMineralPatch(const Point2D &start);
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV);
    bool TryBuildSupplyDepot();
    bool TryBuildBarracks();

private:
    BotConfig config;
};

#endif //BASICSC2BOT_BOT_H