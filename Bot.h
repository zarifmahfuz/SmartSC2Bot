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
    virtual void OnUnitIdle(const Unit *unit) final;

    // this will be get called whenever a new unit is created
    virtual void OnUnitCreated(const Unit *unit) final;

    // this will get called whenever a new unit finishes building
    virtual void OnBuildingConstructionComplete(const Unit *unit) final;

    // counts the current number of units of the specified type
    size_t CountUnitType(UNIT_TYPEID unit_type);

    // finds the nearest requested unit based on euclidean distance
    const Unit *FindNearestRequestedUnit(const Point2D &start, Unit::Alliance alliance, UNIT_TYPEID unit_type);

    // builds a structure at some distance away from the selected builder unit
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV);
    bool TryUpgradeStructure(ABILITY_ID ability_type_for_structure);

    bool TryBuildSupplyDepot();
    bool TryBuildBarracks();
    bool TryBuildRefinery();
    bool TryBuildCommandCenter();
    bool TryUpgradeCommand();

    // trys to attach a Reactor to the n'th Barracks
    bool TryBuildBarracksReactor(size_t n);

    // starts Marine production on the n'th Barracks
    bool TryStartMarineProd(size_t n);

    Tag first_command_center; // tag of the first command center
    
    // issues a command to n number of SCVs
    void CommandSCVs(int n, const Unit *target, ABILITY_ID ability = ABILITY_ID::SMART);

    GameInfo getGameInfo();

    void FindBaseLocations();
    double dist(Point2D p1, Point2D p2);
    bool isMineral(const Unit *u);
    Point2D computeClusterCenter(const std::vector<Point2D> &cluster);
    Point2D chooseNearbyBuildLocation(const Point2D &center, const double &radius);
    double Convert(double degree);
    
  
private:
    BotConfig config;

    std::vector<Point2D> mineralFields;
    std::vector<std::vector<Point2D>> clusters;// clusters representing bases
    std::vector<Point2D> clusterCenters;
    int numClusters;
    std::set<Point2D> test;
    //Bases *bases;
    // represents barracks; index i represents (i+1)'th barracks in the game
    std::vector<Tag> barracks_tags;

    // specifies whether we should start marine production on the first barracks
    bool marine_prod_first_barracks = false;

};

#endif //BASICSC2BOT_BOT_H