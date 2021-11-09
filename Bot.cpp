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
    getGameInfo();
    FindBaseLocations();
    std::cout << "radius: " << units[0]->radius << "poisition (" << units[0]->pos.x << " " << units[0]->pos.y << " " << units[0]->pos.z << ")" << std::endl;
    buildCommand = new BuildCommand();
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
    TryBuildReactorStarport();
    TryBuildMedivac();
    TryResearchCombatShield();
}

size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Bot::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            // train SCVs in the command center
            //TryUpgradeCommand();
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
            break;
        }
        case UNIT_TYPEID::TERRAN_SCV: {
            //if an SCV is idle, tell it mine minerals
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
            //std::cout << "DEBUG: Reactor complete. Start non-stop Marine production\n";
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
                //std::cout << "DEBUG: Sending an SCV to scout\n";
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
            //std::cout << "DEBUG: Assign workers on Refinery\n";
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

Point3D Bot::computeClusterCenter(const std::vector<Point3D> &cluster){
    Point3D center = Point3D(0,0,cluster[0].z);
    for (const Point3D &p: cluster){
        center.x+=p.x;
        center.y+=p.y;
    }
    center.x/=cluster.size();
    center.y/=cluster.size();

    return center;
}

// convert degree to radians
double Bot::Convert(double degree)
{
    double pi = 3.14159265359;
    return (degree * (pi / 180));
}

// given a point and a radius, returns a buildable nearby location
Point3D Bot::chooseNearbyBuildLocation(const Point3D &center, const double &radius){
    Point3D starting_location =  Point3D(center.x+radius,center.y,center.z);
    if (buildCommand->previous_build != Point3D(0,0,0)){
        starting_location = buildCommand->previous_build;
    }
    Point3D build_location;
    // Point p2 = Point(0-radius,0);
    // Point p3 = Point(0,0+radius);
    // Point p4 = Point(0,0-radius);
    double cs;
    double sn;
    double px;
    double py;
    // double angle = buildCommand->angle;
    double angle = 5;
    double iter = 0;
    double _radius = radius;
    bool placeable = false;
    Point3D p1;
    Point3D p2;
    Point3D p3;
    Point3D p4;
    Point3D p5;
    Point3D p6;
    Point3D p7;
    Point3D p8;
    const double command_radius = 2.75;
    double half_diagonal = sqrt(2)*command_radius;
    while (!placeable){
        // change build location
        double theta = Convert(angle);

        cs = cos(theta);
        sn = sin(theta);

        // https://stackoverflow.com/questions/620745/c-rotating-a-vector-around-a-certain-point
        px = ((starting_location.x - center.x) * cs) - ((center.y - starting_location.y) * sn) + center.x;
        py = center.y - ((center.y - starting_location.y) * cs) + ((starting_location.x - center.x) * sn);

        // px = build_location.x * cs - build_location.y * sn;
        // py = build_location.x * sn + build_location.y * cs;

        // build_location.x = px;
        // build_location.y = py;
        ++iter;
        angle+=5;
        if (angle == 360){
            ++_radius;
            starting_location =  Point3D(center.x+_radius,center.y,center.z);
            angle = 0;
        }
        std::cout << "iter: " << iter << " center: ("<< center.x << ", "<< center.y << ") radius: " << _radius << " (" << px << " " << py << ")" << std::endl;
        if (Observation()->IsPlacable(Point2D(px,py)) && Observation()->IsPathable(Point2D(px,py))){
            //placeable = true;
            build_location = Point3D(px,py,center.z);
            p1 = Point3D(build_location.x+half_diagonal,build_location.y+half_diagonal,center.z);
            p2 = Point3D(build_location.x-half_diagonal,build_location.y+half_diagonal,center.z);
            p3 = Point3D(build_location.x-half_diagonal,build_location.y-half_diagonal,center.z);
            p4 = Point3D(build_location.x+half_diagonal,build_location.y-half_diagonal,center.z);
            p5 = Point3D(build_location.x+command_radius,build_location.y,center.z);
            p6 = Point3D(build_location.x,build_location.y+command_radius,center.z);
            p7 = Point3D(build_location.x-command_radius,build_location.y,center.z);
            p8 = Point3D(build_location.x,build_location.y-command_radius,center.z);
            if (Observation()->IsPlacable(p1) && Observation()->IsPlacable(p2) && Observation()->IsPlacable(p3) && Observation()->IsPlacable(p4) &&
            Observation()->IsPlacable(p5) && Observation()->IsPlacable(p6) && Observation()->IsPlacable(p7) && Observation()->IsPlacable(p8)){
                if (Observation()->IsPathable(p1) && Observation()->IsPathable(p2) && Observation()->IsPathable(p3) && Observation()->IsPathable(p4) &&
                Observation()->IsPathable(p5) && Observation()->IsPathable(p6) && Observation()->IsPathable(p7) && Observation()->IsPathable(p8)){
                    std::cout << "found placement! at (" << build_location.x << " " << build_location.y << " " << build_location.z << ") radius: "<< _radius << std::endl;
                    placeable = true;
                }
            }
        }
        //std::cout << "iter: " << iter << std::endl;
    }
    //std::cout << build_location.x << " " << build_location.y << std::endl;
    //std::cout << px << " " << py << std::endl;
    buildCommand->angle = angle;
    buildCommand->previous_build = build_location;
    buildCommand->previous_radius = _radius;

    return build_location;

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

    Point3D closest_mineral; // center of closest mineral field to starting location
    
    // init centers
    Point3D center1 = computeClusterCenter(clusters[0]);
    Point3D center2 = computeClusterCenter(clusters[1]);

    //init min distance found
    double min = Distance3D(center1,center2);
    
    double distance;
    double radius = buildCommand->previous_radius; // arbitrary distance from center of a cluster
    //double radius = 7;
    Point3D build_location;
    switch (ability_type_for_structure)
    {
        case ABILITY_ID::BUILD_COMMANDCENTER:
            // find the center of the cluster that is closest
            for (const Point3D &center2: clusterCenters){
                distance = Distance3D(center1,center2);
                if (distance < min && center2 != clusterCenters[0]){
                    //std::cout << "min distance: " << distance << " between" << center2.x << " " << center2.y << " and " << center1.x << " " << center2.y << std::endl;
                    min = distance;
                    closest_mineral = center2;
                }
            }
            
            buildCommand->closest_mineral = closest_mineral;
            std::cout << "closest mineral " << closest_mineral.x << " " << closest_mineral.y << " " << closest_mineral.z << std::endl;

            build_location = chooseNearbyBuildLocation(closest_mineral,radius);
            // build_location.x = closest_mineral.x + rx * 15.0f;
            // build_location.y = closest_mineral.y + ry * 15.0f;
            //std::cout << "build location " << build_location.x << " " << build_location.y << std::endl;
            Actions()->UnitCommand(builder_unit,
                            ability_type_for_structure,
                            build_location);

            // Actions()->UnitCommand(builder_unit,
            //                 ability_type_for_structure,
            //                 Point2D(build_location.x,build_location.y));
            break;
        
        default:
            Actions()->UnitCommand(builder_unit,
                            ability_type_for_structure,
                            Point2D(builder_unit->pos.x + rx * 15.0f, builder_unit->pos.y + ry * 15.0f));
            break;
    }
    // issue a command to the selected unit
    
    
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
            //std::cout << "DEBUG: Build first supply depot\n";
        }
    } else if (supplyDepotCount == 1) {
        if (observation->GetFoodUsed() >= config.secondSupplyDepot) {
            // build the second supply depot
            buildSupplyDepot = true;
            //std::cout << "DEBUG: Build second supply depot\n";
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
            //std::cout << "DEBUG: Build first barracks\n";
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
            //std::cout << "DEBUG: Build first refinery\n";
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
    //int command_cost = Observation()->GetUnitTypeData()[UnitTypeID(UNIT_TYPEID::TERRAN_COMMANDCENTER)].mineral_cost;
    
    bool buildCommand = false;

    size_t commandCount = CountUnitType(UNIT_TYPEID::TERRAN_COMMANDCENTER);

    size_t command_cost = Observation()->GetUnitTypeData()[UnitTypeID(UNIT_TYPEID::TERRAN_COMMANDCENTER)].mineral_cost;
    size_t mineral_count = Observation()->GetMinerals();
    // build second command center at supply 19 (currently only builds 1 command center)
    if (commandCount==1 && mineral_count >= command_cost ){
        if ( Observation()->GetFoodUsed() >= config.secondCommandCenter ) {
                buildCommand = true;
                std::cout << "DEBUG: Build second command center" << mineral_count << " " << command_cost << "\n";
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
        default:
            break;
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


// for now, gets the start locations
// returns a GameInfo struct
GameInfo Bot::getGameInfo(){
    const ObservationInterface *observation = Observation();
    GameInfo info = observation->GetGameInfo();

    std::vector<Point2D> start_locations = info.start_locations;

    for (const Point2D &p: start_locations){
        std::cout << p.x << " " << p.y << std::endl;
    }
 
    return info;
}

// finds the positions of all minerals where the base locations could be
void Bot::FindBaseLocations(){
    const ObservationInterface *observation = Observation();

    for(auto & u: observation->GetUnits()){
        if (isMineral(u)){
            //std::cout << "mineral field at:" << u->pos.x << " " << u->pos.y << "facing: " << u->facing << std::endl;
            mineralFields.push_back(Point3D(u->pos.x,u->pos.y,u->pos.z));
        }
    }

    // get start location
    Point3D start_location = observation->GetStartLocation();
    double dist_threshold = 10;

    // find starting mineral cluster based on start location
    std::vector<Point3D> start_cluster;

    for (const Point3D &p: mineralFields){
        if (sqrt(DistanceSquared3D(start_location,p))<=dist_threshold){
            start_cluster.push_back(p);
            
       }
    }
    clusters.push_back(start_cluster);
    //test.insert(start_location);
    ++numClusters;
    
    // find other clusters based on distance threshold
    bool skip = false;
    std::vector<Point3D> cluster;
    double distance;
    for (const Point3D &p1: mineralFields){
         // prevent finding duplicate clusters
         for(const std::vector<Point3D> &v: clusters){
            if (!skip){
                for (const Point3D &p: v){
                    if (p==p1){
                        skip = true;
                        break;
                    }
                }
            }
         }
        // find unique cluster based on distance threshold
        if (!skip){
            cluster.push_back(p1);
            for (const Point3D &p2: mineralFields){
                distance  = sqrt(DistanceSquared3D(p1,p2));
                if (distance <= dist_threshold && p1 != p2 ){
                     cluster.push_back(p2);
                }
            }
            clusters.push_back(cluster);
            ++numClusters;
            cluster.clear();
        }
        skip = false;
    }

    // print for debug
    for (const std::vector<Point3D> s: clusters){
         std::cout << "new cluster" << std::endl;
         for (const Point3D &p: s){
             std::cout << p.x << " " << p.y << " " << p.z << std::endl;
         }
    }

    Point2D temp = Observation()->GetUnit(first_command_center)->pos;
    //std::cout << "distance between starting center and field:" << dist(computeClusterCenter(clusters[0]),temp)<< std::endl;

    Point3D center;
    for (const std::vector<Point3D> &p: clusters){
        center = computeClusterCenter(p);
        std::cout << "center: " << center.x << " " << center.y << " " << center.z << std::endl;
        clusterCenters.push_back(center);
    }
    // init bases
    
    // Point2D center;
    // bases = new Bases();
    // for (const std::vector<Point2D> &cluster: clusters){
    //     center = computeClusterCenter(cluster);
    //     bases->bases[center] = false;
    // }
    
    // Point2D start_center = computeClusterCenter(start_cluster);
    // bases->bases[start_center] = true;
    
}


// checks if a unit is a mineral
bool Bot::isMineral(const Unit *u){
    UNIT_TYPEID unit_type = u->unit_type;
    switch (unit_type) 
    {
        case UNIT_TYPEID::NEUTRAL_MINERALFIELD         : return true;
        case UNIT_TYPEID::NEUTRAL_MINERALFIELD750      : return true;
        case UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD     : return true;
        case UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750  : return true;
        case UNIT_TYPEID::NEUTRAL_LABMINERALFIELD		: return true;
        case UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750	: return true;
        default: return false;
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
            //std::cout << "DEBUG: Upgrade " << n << "'th Barracks to Reactor\n";
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
            //std::cout << "DEBUG: Barracks #" << n << " trains Marine\n";
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

bool Bot::TryBuildReactorStarport() {
    const auto *observation = Observation();

    // Only build one Reactor Starport
    if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR) > 0)
        return false;

    // Get existing Starport
    auto starports = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_STARPORT));
    if (starports.empty())
        return false;
    auto *starport = starports[0];

    Actions()->UnitCommand(starport, ABILITY_ID::BUILD_REACTOR_STARPORT);
    std::cout << "DEBUG: Building Reactor Starport\n";
    return true;
}

bool Bot::TryBuildMedivac() {
    const auto *observation = Observation();

    // Only build up to two medivacs at a time
    if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_MEDIVAC) > config.maxMedivacs)
        return false;

    auto reactor_starports = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_STARPORTREACTOR));
    if (reactor_starports.empty())
        return false;
    auto *reactor_starport = reactor_starports[0];

    Actions()->UnitCommand(reactor_starport, ABILITY_ID::TRAIN_MEDIVAC);
    std::cout << "DEBUG: Building Medivac at Reactor Starport\n";
    return true;
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