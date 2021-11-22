#include "Bot.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <string>

Bot::Bot(const BotConfig &config)
        : config(config) {}

void Bot::OnGameStart() {
    std::cout << "Hello World!" << std::endl;
    Units units = Observation()->GetUnits(Unit::Alliance::Self,  IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    // get the tag of the command center the game starts with
    command_center_tags.push_back(units[0]->tag);
    CCStates[command_center_tags[0]] = CommandCenterState::PREUPGRADE_TRAINSCV;
    std::cout << "pos: " << units[0]->pos.x << " " << units[0]->pos.y  << std::endl;
    FindBaseLocations();
    BuildMap = std::map<std::string,BuildInfo>();
    ramp_location = Point3D(0,0,0);
    starting_location = units[0]->pos;
}

void Bot::OnGameEnd(){
}

void Bot::OnStep() {
    SupplyDepotHandler();
    
    CommandCenterHandler();

    BarracksHandler();

    RefineryHandler();

    EBayHandler();

    findNearbyRamp();
    // TryBuildEngineeringBay();
    // TryResearchInfantryWeapons();
    // TryBuildMissileTurret();
    // TryBuildStarport();
    // TryBuildReactorStarport();
    // TryBuildMedivac();
    // TryResearchCombatShield();
    
}

size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Bot::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            // if an SCV is idle, tell it mine minerals
            const Unit *mineral_target = FindNearestRequestedUnit(unit->pos, Unit::Alliance::Neutral, UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            if (!mineral_target) {
                break;
            }
            // the ability type SMART is equivalent to a right click when you have a unit selected
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS: {
            // Barracks has just finished building
            // do not add the tag if it is already present
            auto p = std::find( begin(barracks_tags), end(barracks_tags), unit->tag);
            if (p == end(barracks_tags)) {
                // add the barracks tag to barracks_tags
                barracks_tags.push_back(unit->tag);
            }
            break;
        }

        case UNIT_TYPEID::TERRAN_ENGINEERINGBAY: {
            // E-Bay just finished building
            // do not add the tag if it is already present
            auto p = std::find( begin(e_bay_tags), end(e_bay_tags), unit->tag);
            if (p == end(e_bay_tags)) {
                // add the E-Bay tag to barracks_tags
                e_bay_tags.push_back(unit->tag);
            }
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
                std::cout << "DEBUG: Sending an SCV to scout\n";
                if (&scout == nullptr){
                    scout = *unit;
                }
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:{
            // check if the tag is already in command_center_tags
            // if it's not, add it and init its state
            // this check is only necessary because OnUnitCreated() is also called for the first command center
            bool add_cc = true;
            for (const Tag &cc: command_center_tags){
                if (cc == unit->tag){
                    add_cc = false;
                }
            }
            if (add_cc){
                command_center_tags.push_back(unit->tag);
                CCStates[unit->tag] = CommandCenterState::BUILDCC;
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
            // do not add duplicate tags
            auto p = std::find( begin(refinery_tags), end(refinery_tags), unit->tag);
            if (p == end(refinery_tags)) {
                // add the refinery tag to refinery_tags
                refinery_tags.push_back(unit->tag);
            }
            ChangeRefineryState();
            break;
        }
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:{
            std::cout << "DEBUG: CC finished building" << std::endl;
            ChangeCCState(unit->tag); //command center finished building, so change its state
            break;
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

bool Bot::canAffordUnit(UNIT_TYPEID unitType){
    int mineral_cost = Observation()->GetUnitTypeData()[UnitTypeID(unitType)].mineral_cost;
    int vespene_cost = Observation()->GetUnitTypeData()[UnitTypeID(unitType)].vespene_cost;
    int mineral_count = Observation()->GetMinerals();
    int vespene_count = Observation()->GetVespene();
    switch(unitType){
        // special case for the orbital command, doesn't apply for other units in our build order
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:{
            int command_cost = Observation()->GetUnitTypeData()[UnitTypeID(UNIT_TYPEID::TERRAN_COMMANDCENTER)].mineral_cost;
            if (mineral_count >= (mineral_cost - command_cost )){
                return true;
            }
            break;
        }
        default:{
            if (mineral_count>=mineral_cost && vespene_count>=vespene_cost){
                return true;
            }
            break;
        }     
    }
    return false;
}

bool Bot::canAffordUpgrade(UPGRADE_ID upgradeID){
    int mineral_cost = Observation()->GetUpgradeData()[UpgradeID(upgradeID)].mineral_cost;
    int vespene_cost = Observation()->GetUpgradeData()[UpgradeID(upgradeID)].vespene_cost;
    int mineral_count = Observation()->GetMinerals();
    int vespene_count = Observation()->GetVespene();
    if (mineral_count>=mineral_cost && vespene_count>=vespene_cost){
                return true;
    }
    return false;
}

bool Bot::eraseTag(std::vector<Tag> &v, const Tag &tag){
    auto it = begin(v);
    for(; it < end(v); ++it){
        if (*it == tag){
            v.erase(it);
            return true;
        }
    }
    return false;
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
Point3D Bot::chooseNearbyBuildLocation(const Point3D &center, const double &radius, ABILITY_ID ability_type_for_structure, std::string n){
    BuildInfo *buildInfo = &BuildMap[n];

    Point3D starting_location =  Point3D(center.x+radius,center.y,center.z);
    Point3D build_location;
    double cs, sn, px, py;
    double angle = buildInfo->angle;
    double iter = 0;
    double _radius = radius;
    bool placeable = false;
    Point3D p1, p2, p3, p4, p5, p6, p7, p8;
    double unit_radius = buildInfo->unit_radius;
    double half_diagonal = sqrt(2)*unit_radius;
    while (!placeable){
        // change build location
        double theta = Convert(angle);

        cs = cos(theta);
        sn = sin(theta);

        // https://stackoverflow.com/questions/620745/c-rotating-a-vector-around-a-certain-point
        px = ((starting_location.x - center.x) * cs) - ((center.y - starting_location.y) * sn) + center.x;
        py = center.y - ((center.y - starting_location.y) * cs) + ((starting_location.x - center.x) * sn);

        ++iter;
        angle+=5;
        if (angle == 360){
            if (_radius<25){
                ++_radius;}
            else{
                _radius = buildInfo->default_radius;
            }
            starting_location =  Point3D(center.x+_radius,center.y,center.z);
            angle = 0;
        }
        std::cout << " iter: " << iter << " center: ("<< center.x << ", "<< center.y << ") radius: " << _radius << " (" << px << " " << py << ") " << "distance: " << Distance2D(Point2D(px,py),center) << std::endl;
        std::cout << BuildMap[n].angle << " " << BuildMap[n].unit_radius << " " << BuildMap[n].previous_radius << std::endl;
        if (Observation()->IsPlacable(Point2D(px,py)) && Observation()->IsPathable(Point2D(px,py))){
            build_location = Point3D(px,py,center.z);
            p1 = Point3D(build_location.x+half_diagonal,build_location.y+half_diagonal,center.z);
            p2 = Point3D(build_location.x-half_diagonal,build_location.y+half_diagonal,center.z);
            p3 = Point3D(build_location.x-half_diagonal,build_location.y-half_diagonal,center.z);
            p4 = Point3D(build_location.x+half_diagonal,build_location.y-half_diagonal,center.z);
            p5 = Point3D(build_location.x+unit_radius,build_location.y,center.z);
            p6 = Point3D(build_location.x,build_location.y+unit_radius,center.z);
            p7 = Point3D(build_location.x-unit_radius,build_location.y,center.z);
            p8 = Point3D(build_location.x,build_location.y-unit_radius,center.z);
            if (Observation()->IsPlacable(p1) && Observation()->IsPlacable(p2) && Observation()->IsPlacable(p3) && Observation()->IsPlacable(p4) &&
            Observation()->IsPlacable(p5) && Observation()->IsPlacable(p6) && Observation()->IsPlacable(p7) && Observation()->IsPlacable(p8)){
                if (Observation()->IsPathable(p1) && Observation()->IsPathable(p2) && Observation()->IsPathable(p3) && Observation()->IsPathable(p4) &&
                Observation()->IsPathable(p5) && Observation()->IsPathable(p6) && Observation()->IsPathable(p7) && Observation()->IsPathable(p8)){
                    // std::cout << "found placement! at (" << build_location.x << " " << build_location.y << " " << build_location.z << ") radius: "<< _radius << std::endl;
                    placeable = true;
                }
            }
        }
    }

    buildInfo->angle = angle;
    buildInfo->previous_build = build_location;
    buildInfo->previous_radius = _radius;

    return build_location;

}

void Bot::findNearbyRamp(){
    if (&scout != nullptr){
        int threshold = 2;
        if (scout.pos.z <= (starting_location.z-threshold) && ramp_location != Point3D(0,0,0)){
            ramp_location = scout.pos;
            Actions()->UnitCommand(&scout, ABILITY_ID::SMART, ramp_location);
            std::cout << "ramp_location " << ramp_location.x << " " << ramp_location.y << " " << ramp_location.z << std::endl;
        }
    }
}

bool Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type, bool simult, std::string n) {
    const ObservationInterface *observation = Observation();
    
    // get a unit to build the structure
    const Unit *builder_unit = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    for (const auto &unit : units) {
        for (const auto &order : unit->orders) {
            if (order.ability_id == ability_type_for_structure) {
                // a structure of this type is already building
                if (simult) {
                    // get a unit that's not already building this structure
                    break;
                } else {
                    // will not allow building two units of the same type simultaneously
                    return false;
                }
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
    Point3D center1 = clusterCenters[0];
    Point3D center2 = clusterCenters[1];
    
    //init min distance found
    double min = Distance3D(center1,center2);
    
    double distance;
     // arbitrary distance from center of a cluster
    
    Point3D build_location;
    double radius;
    Point3D center_pos;
    switch (ability_type_for_structure)
    {
        case ABILITY_ID::BUILD_COMMANDCENTER:
            if (BuildMap.find("command") == end(BuildMap)){
                BuildMap["command"] = BuildCommandInfo();
            }
            // find the center of the cluster that is closest to starting location
            for (const Point3D &center2: clusterCenters){
                distance = Distance3D(center1,center2);
                if (distance < min && center2 != clusterCenters[0]){
                    //std::cout << "min distance: " << distance << " between" << center2.x << " " << center2.y << " and " << center1.x << " " << center2.y << std::endl;
                    min = distance;
                    closest_mineral = center2;
                }
            }
            radius = BuildMap["command"].previous_radius;
            //buildCommand->closest_mineral = closest_mineral;
            // std::cout << "closest mineral " << closest_mineral.x << " " << closest_mineral.y << " " << closest_mineral.z << std::endl;

            build_location = chooseNearbyBuildLocation(closest_mineral,radius,ability_type_for_structure,"command");
        
            Actions()->UnitCommand(builder_unit,
                            ability_type_for_structure,
                            build_location);

            break;
        case ABILITY_ID::BUILD_BARRACKS:
            if (BuildMap.find(n) == end(BuildMap)){
                std::cout << n << std::endl;
                BuildMap[n] = BuildBarracksInfo();
            }
            radius = BuildMap[n].previous_radius;
            center_pos = Observation()->GetUnit(command_center_tags[0])->pos;
            build_location = chooseNearbyBuildLocation(center_pos,radius,ability_type_for_structure,n);
        
            Actions()->UnitCommand(builder_unit,
                            ability_type_for_structure,
                            build_location);
            break;
        default:
            Actions()->UnitCommand(builder_unit,
                            ability_type_for_structure,
                            Point2D(builder_unit->pos.x + rx * 15.0f, builder_unit->pos.y + ry * 15.0f));
            break;
    }
    
    return true;
}

bool Bot::CommandSCVs(int n, const Unit *target, ABILITY_ID ability) {
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
        return true;
    }
    return false;
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

// finds the positions of all minerals where the base locations could be
void Bot::FindBaseLocations(){
    // vector storing the locations of all mineral fields
    std::vector<Point3D> mineralFields;

    // vector storing clusters of mineral fields
    std::vector<std::vector<Point3D>> clusters;

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


    Point2D temp = Observation()->GetUnit(command_center_tags[0])->pos;
    //std::cout << "distance between starting center and field:" << dist(computeClusterCenter(clusters[0]),temp)<< std::endl;

    Point3D center;
    for (const std::vector<Point3D> &p: clusters){
        center = computeClusterCenter(p);
        //std::cout << "center: " << center.x << " " << center.y << " " << center.z << std::endl;
        clusterCenters.push_back(center);
    }
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