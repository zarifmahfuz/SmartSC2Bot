#ifndef BASICSC2BOT_BOT_H
#define BASICSC2BOT_BOT_H

#include <sc2api/sc2_api.h>
#include <sc2api/sc2_unit_filters.h>
#include "BotConfig.h"
#include <map>
#include <utility>
#include <math.h>
#include "BuildInfo.h"

using namespace sc2;

// states mapping the action we are trying to do related to a Supply Depot
enum SupplyDepotState { FIRST, SECOND, THIRD, CONT };

// states mapping the action we are trying to do with a Refinery
enum RefineryState { REFINERY_FIRST, ASSIGN_WORKERS, REFINERY_IDLE };

// states representing actions taken by Barracks
enum class BarracksState { BUILDING, BUILDING_TECH_LAB, BUILDING_REACTOR, PRODUCING_MARINES, PRODUCING_MARAUDERS };

// states representing actions taken by Barracks Tech Lab
enum class BarracksTechLabState { NONE, BUILDING, RESEARCHING_STIMPACK, RESEARCHING_COMBAT_SHIELD, DONE };

// states representing actions taken by the first Command Center

enum CommandCenterState { BUILDCC, PREUPGRADE_TRAINSCV, OC, POSTUPGRADE_TRAINSCV };

// states reprenting actions taken by the first Engineering Bay
enum EBayState {EBAYBUILD, INFANTRYWEAPONSUPGRADELEVEL1};

// number of seconds per game loop from Observation()->GetGameLoop()
const float SECONDS_PER_GAME_LOOP = 1 / 22.4F;

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

    // called whenever an upgrade completes
    virtual void OnUpgradeCompleted(UpgradeID) final;

    // called when an enemy unit enters our vision from out of fog of war
    virtual void OnUnitEnterVision(const Unit *) override;

    // counts the current number of units of the specified type
    size_t CountUnitType(UNIT_TYPEID unit_type);

    // finds the nearest requested unit based on euclidean distance
    const Unit *FindNearestRequestedUnit(const Point2D &start, Unit::Alliance alliance, UNIT_TYPEID unit_type);

    // builds a structure at some distance away from the selected builder unit
    // simult is set to true when you want to allow building a unit when another unit of the same type is under construction
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV, bool simult = false, std::string n = " ");

    bool TryBuildCommandCenter();
    
    // issues a command to n number of SCVs - returns true if the command was successful, false otherwise
    bool CommandSCVs(int n, const Unit *target, ABILITY_ID ability = ABILITY_ID::SMART);
  
private:
    BotConfig config;

    double num_game_loops_elapsed = 0;
    double const num_game_loops_per_second = 22.4;

    std::string ai_difficulty;
    std::string map_name;


    // the location of each of the expansions
    std::unique_ptr<std::vector<Point3D>> expansion_locations = nullptr;

    // centers of all clusters of mineral fields
    std::vector<Point3D> clusterCenters;

    // number of mineral field clusters
    int numClusters;

    // struct to store info related to building command centers in the correct location
    struct BuildCommandInfo:public BuildInfo{
        BuildCommandInfo(){
            previous_radius = 6;
            default_radius = 6;
            unit_radius = 3;
        }
    };

    // struct to store info related to building barracks in the correct location
    struct BuildBarracksInfo:public BuildInfo{
        BuildBarracksInfo(){   
            previous_radius = 12;
            unit_radius = 3;
            default_radius = 12;
        }
    };

    // this map stores the build info of a building to be built identified by a string
    std::map<std::string,BuildInfo> BuildMap;

    // finds the locations of all bases in the map
    void FindBaseLocations();

    // determines if a unit is a mineral field
    bool isMineral(const Unit *u);

    // computes the center of a cluster of mineral feilds
    Point3D computeClusterCenter(const std::vector<Point3D> &cluster);

    // chooses a nearby location to a mineral field to build on
    Point2D chooseNearbyBuildLocation(const Point3D &center, const double &radius, ABILITY_ID ability_type_for_structure, std::string n = " ");

    // find nearby ramp location
    void findNearbyRamp();

    // convert degree to radian
    double Convert(double degree);

    // erase tag from a vector
    bool eraseTag(std::vector<Tag> &v, const Tag &tag);

    // returns true if there is enough minerals and vespene to afford a unit
    bool canAffordUnit(UNIT_TYPEID unitType);

    // returns true if there is enough minerals and vespene to afford an upgrade
    bool canAffordUpgrade(UPGRADE_ID upgrade);


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
    std::vector<Tag> barracks_tags;
    std::vector<BarracksState> barracks_states;
    std::vector<BarracksTechLabState> barracks_tech_lab_states;

    // If the stimpack upgrade has completed
    bool have_stimpack = false;
    // If the combat shield upgrade has completed
    bool have_combat_shield = false;

    // try building a Barracks
    void TryBuildingBarracks();

    // trys to attach a Reactor to a Barracks
    void TryBuildingBarracksReactor(const Unit *barracks);

    // trys to attach a Tech Lab to the n'th Barracks
    void TryBuildingBarracksTechLab(const Unit *barracks);
    
    // trys to research Stimpack at a Barracks Tech Lab
    void TryResearchingStimpack(const Unit *tech_lab);

    // trys to research Combat Shield at a Barracks Tech Lab
    void TryResearchingCombatShield(const Unit *tech_lab);

    // trys to produce a Marine at a Barracks
    void TryProducingMarine(const Unit *barracks);

    // Try to produce a Marauder at a Barracks
    void TryProducingMarauder(const Unit *barracks);

    // handles the states and actions of all the Barracks in the game
    void BarracksHandler();

    // handles the state machine of a Barracks with Tech Lab
    void BarracksTechLabHandler(const Unit *barracks, BarracksTechLabState &state);


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

    // gets the maximum number of SCVs that should be trained during the game
    int maxSCVs(const int &maxSimulScouts);

    // ------------------------ REFINERY ----------------------------
    std::vector<Tag> refinery_tags;
    RefineryState refinery_state = RefineryState::REFINERY_FIRST;
    void ChangeRefineryState();
    void RefineryHandler();
    // parameter can be "first", "second", ...
    bool TryBuildRefinery(std::string &refinery_);
    // assigns a target amount of workers to a given Refinery, should be between [0, 3]
    // returns true if workers were assigned, false otherwise
    bool AssignWorkersToRefinery(const Tag &tag, int target_workers);

    // ---------------------ENGINEERING BAY -------------------------
    std::vector<Tag> e_bay_tags;
    EBayState first_e_bay_state = EBayState::EBAYBUILD;

    void EBayHandler();

    bool TryBuildEBay(std::string &ebays_);

    bool TryInfantryWeaponsUpgrade(size_t n);

    void ChangeFirstEbayState();

    // -------------------------- ARMY UNITS ------------------------
    // true iff army units should attack enemy units when they idle, false otherwise.
    bool units_should_attack = false;

    // true iff we have found an enemy unit.
    bool found_enemy = false;

    // true iff we were just defending our base against at least one enemy.
    bool just_defended = false;

    // Get a list of all of our army units.
    Units GetArmyUnits();

    // Checks if an attack should be made and performs an attack if so.
    void AttackHandler();

    // Checks if we should defend our base and performs a defensive action if so.
    void DefendHandler();

    // Command a unit to perform an attack on the enemy.
    void CommandToAttack(const Unit *attacking_unit, const Units &enemy_units);

    // Command a unit to search for enemy units.
    void CommandToSearchForEnemies(const Unit *);

    // -------------------------- SCOUTING --------------------------
    // The tag of the scouting SCV, or 0 if there is none.
    Tag scouting_scv = 0;

    // True iff the scouting SCV is currently returning to the base. Undefined if there is no scouting SCV.
    bool scouting_scv_returning = false;

    // Send a unit to scout for enemy buildings
    void SendScout();
};

#endif //BASICSC2BOT_BOT_H