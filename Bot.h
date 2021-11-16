#ifndef BASICSC2BOT_BOT_H
#define BASICSC2BOT_BOT_H

#include <sc2api/sc2_api.h>
#include "BotConfig.h"
#include <map>

using namespace sc2;

// states representing which supply depot to build .. e.g. FIRST means the first supply depot has been built
enum SupplyDepotState { FIRST, SECOND, THIRD, CONT };

// states representing actions taken by the first, second and third Barracks
enum BarracksState { BUILD, TECHLAB, REACTOR, STIMPACK, MARINEPROD };

// states representing actions taken by the first Command Center

enum CommandCenterState { BUILDCC, PREUPGRADE_TRAINSCV, OC, POSTUPGRADE_TRAINSCV };

// states reprenting actions taken by the first Engineering Bay
enum EBayState {EBAYBUILD, INFANTRYWEAPONSUPGRADELEVEL1};

class Bot : public Agent {
public:
    explicit Bot(const BotConfig &config);

    virtual void OnGameStart() final;
    virtual void OnGameEnd() final;
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
    // simult is set to true when you want to allow building a unit when another unit of the same type is under construction
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV, bool simult = false);

    bool TryBuildRefinery();
    bool TryBuildCommandCenter();
    
    // issues a command to n number of SCVs
    void CommandSCVs(int n, const Unit *target, ABILITY_ID ability = ABILITY_ID::SMART);
  
private:
    BotConfig config;

    // centers of all clusters of mineral fields
    std::vector<Point3D> clusterCenters;

    // number of mineral field clusters
    int numClusters;

    // struct to store info related to building command centers in the correct location
    struct BuildCommandInfo{
        Point3D previous_build;
        Point3D closest_mineral;
        int previous_radius;
        int iter;
        double angle;
        BuildCommandInfo(){
            previous_build = Point3D(0,0,0);
            closest_mineral = Point3D(0,0,0);
            previous_radius = 6;
            iter = 0;
            angle = 5;
        }
    };
    BuildCommandInfo *buildCommand;

    // finds the locations of all bases in the map
    void FindBaseLocations();

    // determines if a unit is a mineral field
    bool isMineral(const Unit *u);

    // computes the center of a cluster of mineral feilds
    Point3D computeClusterCenter(const std::vector<Point3D> &cluster);

    // chooses a nearby location to a mineral field to build on
    Point3D chooseNearbyBuildLocation(const Point3D &center, const double &radius);

    // convert degree to radian
    double Convert(double degree);

    // erase tag from a vector
    bool eraseTag(std::vector<Tag> &v, const Tag &tag);

    // returns true if there is enough minerals and vespene to afford a unit
    bool canAffordUnit(UNIT_TYPEID unitType);

    // returns true if there is enough minerals and vespene to afford an upgrade
    bool canAffordUpgrade(UPGRADE_ID upgrade);

    // Try to build an Engineering Bay.
    bool TryBuildEngineeringBay();

    // Return the first built Engineering Bay if it exists, or nullptr if none exists.
    const Unit *GetEngineeringBay();

    // Try to research the Infantry Weapons Level 1 upgrade using the Engineering Bay.
    bool TryResearchInfantryWeapons();

    // Try to build a Missile Turret using the Engineering Bay.
    bool TryBuildMissileTurret();

    // Try to build a Starport.
    bool TryBuildStarport();

    // Try to build a Reactor Starport on the existing Starport.
    bool TryBuildReactorStarport();

    // Try to build a Medivac
    bool TryBuildMedivac();

    // Try to research the Combat Shield upgrade on the Barracks' Tech Lab.
    bool TryResearchCombatShield();

    

    // ----------------- SUPPLY DEPOT ----------------
    // represents supply depots; index i represents (i+1)'th supply depot in the game
    // at this point, this vector is empty because I don't intend on doing anything with the supply depots
    std::vector<Tag> supply_depot_tags;
    SupplyDepotState supply_depot_state = SupplyDepotState::FIRST;

    bool TryBuildSupplyDepot(std::string &depot_);
    
    // handles the states of Supply Depot
    void SupplyDepotHandler();

    // changes states for the SupplyDepot state machine
    void ChangeSupplyDepotState();


    // ----------------- BARRACKS ----------------
    // represents barracks; index i represents (i+1)'th barracks in the game
    std::vector<Tag> barracks_tags;
    BarracksState first_barracks_state = BarracksState::BUILD;
    BarracksState second_barracks_state = BarracksState::BUILD;
    BarracksState third_barracks_state = BarracksState::BUILD;

    // build the n'th Barracks; barracks_ = "first"/"second"/"third"
    bool TryBuildBarracks(std::string &barracks_);

    // trys to attach a Reactor to the n'th Barracks
    bool TryBuildBarracksReactor(size_t n);

    // trys to attach a Tech Lab to the n'th Barracks
    bool TryBuildBarracksTechLab(size_t n);
    
    // trys to research Stimpack at the n'th Barracks - returns true if successful, false otherwise
    bool TryResearchBarracksStimpack(size_t n);

    // starts Marine production on the n'th Barracks
    // if Reactor is attached to a Barracks, it produces two units simultaneously
    bool TryStartMarineProd(size_t n, bool has_reactor);

    // handles the states and actions of all the Barracks in the game
    void BarracksHandler();

    // changes states for the first Barracks
    void ChangeFirstBarracksState();
    // changes states for the second Barracks
    void ChangeSecondBarracksState();
    // changes states for the third Barracks
    void ChangeThirdBarracksState();

    // ------------------------ COMMAND CENTER --------------------------
    // represents command centers; index i represents (i+1)'th command center in the game
    std::vector<Tag> command_center_tags;
    bool first_cc_drop_mules = false;

    // keeps track of the state for each command center
    std::map<Tag,CommandCenterState> CCStates;

    // changes states for CC
    void ChangeCCState(Tag cc);

    // handles the states and actions of all the CCs in the game
    void CommandCenterHandler();

    // upgrades the n'th CC to and Orbital Command
    bool TryUpgradeToOC(size_t n);

    // ------------------------ REFINERY ----------------------------


    // ---------------------ENGINEERING BAY -------------------------
    std::vector<Tag> e_bay_tags;
    EBayState first_e_bay_state = EBayState::EBAYBUILD;

    void EBayHandler();

    bool TryBuildEBay(std::string &ebays_);

    bool TryInfantryWeaponsUpgrade(size_t n);

    void ChangeFirstEbayState();
};

#endif //BASICSC2BOT_BOT_H