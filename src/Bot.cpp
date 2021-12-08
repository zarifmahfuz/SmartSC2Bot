#include <sc2lib/sc2_search.h>

#include "Bot.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <string>

Bot::Bot(const BotConfig &config)
        : config(config) {}

void Bot::OnGameStart() {
    const auto *observation = Observation();

    // Calculate expansion locations and cache the result
    expansion_locations = std::make_unique<std::vector<Point3D>>(
            std::move(search::CalculateExpansionLocations(observation, Query())));

    GameInfo gameInfo = Observation()->GetGameInfo();
    map_name = gameInfo.map_name;

    // AI Race
    switch (gameInfo.player_info[1].race_requested) {
        case Terran: {
            std::cout << "TERRAN" << std::endl;
            break;
        }
        case Zerg: {
            std::cout << "ZERG" << std::endl;
            break;
        }
        case Protoss: {
            std::cout << "PROTOSS" << std::endl;
            break;
        }
        case Random: {
            std::cout << "RANDOM" << std::endl;
            break;
        }

    }

    // AI difficulty
    switch (gameInfo.player_info[1].difficulty) {
        case VeryEasy: {
            ai_difficulty = "VERY EASY";
            break;
        }
        case Easy: {
            ai_difficulty = "EASY";
            break;
        }
        case Medium: {
            ai_difficulty = "MEDIUM";
            break;
        }
        case MediumHard: {
            ai_difficulty = "MEDIUM HARD";
            break;
        }
        case Hard: {
            ai_difficulty = "HARD";
            break;
        }
        case HardVeryHard: {
            ai_difficulty = "HARD VERY HARD";
            break;
        }
        case VeryHard: {
            ai_difficulty = "VERY HARD";
            break;
        }
        case CheatVision: {
            ai_difficulty = "CHEAT VISION";
            break;
        }
        case CheatMoney: {
            ai_difficulty = "CHEAT MONEY";
            break;
        }
        case CheatInsane: {
            ai_difficulty = "CHEAT INSANE";
            break;
        }
    }

    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    // get the tag of the command center the game starts with
    command_center_tags.push_back(units[0]->tag);
    CCStates[command_center_tags[0]] = CommandCenterState::PREUPGRADE_TRAINSCV;
    std::cout << "pos: " << units[0]->pos.x << " " << units[0]->pos.y << std::endl;
    FindBaseLocations();
    BuildMap = std::map<std::string, BuildInfo>();

    SendScout();
}

void Bot::OnGameEnd() {

    std::vector<PlayerResult> results = Observation()->GetResults();

    // print map name
    std::cout << map_name << std::endl;

    // print results
    if (!results.empty()) {
        std::cout << "RESULTS: " << std::endl;

        switch (results[0].result) {

            case Win: {
                std::cout << "WIN" << std::endl;
                break;
            }

            case Loss: {
                std::cout << "LOSS" << std::endl;
                break;
            }

            case Tie: {
                std::cout << "TIE" << std::endl;
                break;
            }
            case Undecided: {
                std::cout << "UNDECIDED" << std::endl;
                break;
            }

            default: {
                std::cout << "DEFAULT" << std::endl;
                break;
            }


        }

    }

    // print AI difficulty
    std::cout << ai_difficulty << std::endl;

    // print number of seconds of game time elapsed
    std::cout << num_game_loops_elapsed / num_game_loops_per_second << std::endl;
}

void Bot::OnStep() {

    ++num_game_loops_elapsed;

    SupplyDepotHandler();

    CommandCenterHandler();

    BarracksHandler();

    RefineryHandler();

    EBayHandler();

    AttackHandler();

    DefendHandler();
}

size_t Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

void Bot::OnUnitIdle(const Unit *unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::TERRAN_SCV: {
            // Make scouting SCV no longer the scouting SCV when it becomes idle
            if (unit->tag == scouting_scv) {
                scouting_scv = 0;
            }

            // if an SCV is idle, tell it mine minerals
            const Unit *mineral_target = FindNearestRequestedUnit(unit->pos, Unit::Alliance::Neutral,
                                                                  UNIT_TYPEID::NEUTRAL_MINERALFIELD);
            if (!mineral_target) {
                break;
            }
            // the ability type SMART is equivalent to a right click when you have a unit selected
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            break;
        }
        case UNIT_TYPEID::TERRAN_ENGINEERINGBAY: {
            // E-Bay just finished building
            // do not add the tag if it is already present
            auto p = std::find(begin(e_bay_tags), end(e_bay_tags), unit->tag);
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
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            // check if the tag is already in command_center_tags
            // if it's not, add it and init its state
            // this check is only necessary because OnUnitCreated() is also called for the first command center
            bool add_cc = true;
            for (const Tag &cc: command_center_tags) {
                if (cc == unit->tag) {
                    add_cc = false;
                }
            }
            if (add_cc) {
                command_center_tags.push_back(unit->tag);
                CCStates[unit->tag] = CommandCenterState::BUILDCC;
            }
            break;
        }
        case UNIT_TYPEID::TERRAN_BARRACKS:
            std::cout << "DEBUG: Started building a Barracks (i=" << barracks_tags.size() << ")\n";
            barracks_tags.push_back(unit->tag);
            barracks_states.push_back(BarracksState::BUILDING);
            barracks_tech_lab_states.push_back(BarracksTechLabState::NONE);
            break;
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
            std::cout << "DEBUG: Started building a Barracks Tech Lab\n";
            break;
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
            std::cout << "DEBUG: Started building a Barracks Reactor\n";
            break;
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_MARAUDER: {
            // Move the newly-created marine or marauder to the rally point, which is the location of the command center
            const auto *cc = Observation()->GetUnit(command_center_tags[0]);
            if (cc) {
                Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVE, cc->pos);
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
            auto p = std::find(begin(refinery_tags), end(refinery_tags), unit->tag);
            if (p == end(refinery_tags)) {
                // add the refinery tag to refinery_tags
                refinery_tags.push_back(unit->tag);
            }
            ChangeRefineryState();
            break;
        }
        case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
            // std::cout << "DEBUG: CC finished building" << std::endl;
            ChangeCCState(unit->tag); //command center finished building, so change its state
            break;
        }
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT: {
            ChangeSupplyDepotState();
            break;
        }
        default: {
            break;
        }
    }
}

void Bot::OnUpgradeCompleted(UpgradeID upgradeId) {
    if (upgradeId == UPGRADE_ID::STIMPACK)
        have_stimpack = true;
    else if (upgradeId == UPGRADE_ID::SHIELDWALL)
        have_combat_shield = true;
}

const Unit *Bot::FindNearestRequestedUnit(const Point2D &start, Unit::Alliance alliance, UNIT_TYPEID unit_type) {
    // get all units of the specified alliance
    Units units = Observation()->GetUnits(alliance);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;

    // iterate over all the units and find the closest unit matching the unit type
    for (const auto &u: units) {
        if (u == nullptr) {
            continue;
        }
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

bool Bot::canAffordUnit(UNIT_TYPEID unitType) {
    int mineral_cost = Observation()->GetUnitTypeData()[UnitTypeID(unitType)].mineral_cost;
    int vespene_cost = Observation()->GetUnitTypeData()[UnitTypeID(unitType)].vespene_cost;
    int mineral_count = Observation()->GetMinerals();
    int vespene_count = Observation()->GetVespene();
    switch (unitType) {
        // special case for the orbital command, doesn't apply for other units in our build order
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND: {
            int command_cost = Observation()->GetUnitTypeData()[UnitTypeID(
                    UNIT_TYPEID::TERRAN_COMMANDCENTER)].mineral_cost;
            if (mineral_count >= (mineral_cost - command_cost)) {
                return true;
            }
            break;
        }
        default: {
            if (mineral_count >= mineral_cost && vespene_count >= vespene_cost) {
                return true;
            }
            break;
        }
    }
    return false;
}

bool Bot::canAffordUpgrade(UPGRADE_ID upgradeID) {
    int mineral_cost = Observation()->GetUpgradeData()[UpgradeID(upgradeID)].mineral_cost;
    int vespene_cost = Observation()->GetUpgradeData()[UpgradeID(upgradeID)].vespene_cost;
    int mineral_count = Observation()->GetMinerals();
    int vespene_count = Observation()->GetVespene();
    if (mineral_count >= mineral_cost && vespene_count >= vespene_cost) {
        return true;
    }
    return false;
}

bool Bot::eraseTag(std::vector<Tag> &v, const Tag &tag) {
    auto it = begin(v);
    for (; it < end(v); ++it) {
        if (*it == tag) {
            v.erase(it);
            return true;
        }
    }
    return false;
}

Point3D Bot::computeClusterCenter(const std::vector<Point3D> &cluster) {
    Point3D center = Point3D(0, 0, cluster[0].z);
    for (const Point3D &p: cluster) {
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= cluster.size();
    center.y /= cluster.size();

    return center;
}

// convert degree to radians
double Bot::Convert(double degree) {
    double pi = 3.14159265359;
    return (degree * (pi / 180));
}

// given a point and a radius, returns a buildable nearby location
Point2D
Bot::chooseNearbyBuildLocation(const Point3D &center, const double &radius, ABILITY_ID ability_type_for_structure,
                               std::string n) {

    // load BuildInfo struct for barracks or command center
    BuildInfo *buildInfo = &BuildMap[n];

    // start with a vector
    Point2D starting_location = Point2D(center.x + radius, center.y);

    Point2D build_location;
    double cs, sn, px, py;

    // angle to rotate the starting location vector by
    double angle = buildInfo->angle;

    double iter = 0;
    double _radius = radius;
    bool placeable = false;

    double unit_radius = buildInfo->unit_radius;

    // keep rotating a vector around a center point until a location is found
    while (!placeable) {
        // convert degrees to radians
        double theta = Convert(angle);

        // compute cos and sin for the chosen angle
        cs = cos(theta);
        sn = sin(theta);

        // https://stackoverflow.com/questions/620745/c-rotating-a-vector-around-a-certain-point
        // rotate a vector around a point by an angle theta
        px = ((starting_location.x - center.x) * cs) - ((center.y - starting_location.y) * sn) + center.x;
        py = center.y - ((center.y - starting_location.y) * cs) + ((starting_location.x - center.x) * sn);

        ++iter;

        // increase angle every iteration
        angle += 5;

        // after rotating the vector by 360 degrees, increase the radius and keep looking for a placement location
        if (angle == 360) {
            // reset radius if it gets too large
            if (_radius < 25) {
                ++_radius;
            } else {
                _radius = buildInfo->default_radius;
            }
            // adjust the starting vector
            starting_location = Point3D(center.x + _radius, center.y, center.z);
            // reset angle
            angle = 0;
        }

        // if the vector (px,py) is a placable and pathable point, choose it as a possible build_location
        if (Observation()->IsPlacable(Point2D(px, py)) && Observation()->IsPathable(Point2D(px, py))) {


            build_location = Point2D(px, py);

            // vector to rotate around the build location to check if there is enough space for a structure
            Point2D check_location = Point2D(build_location.x + buildInfo->unit_radius, build_location.y);
            double inner_angle = 5;
            placeable = true;

            // place barracks away from each other to allow for attaching techlab/reactor without conflict
            if (ability_type_for_structure == ABILITY_ID::BUILD_BARRACKS && !barracks_tags.empty()) {
                for (const Tag &t: barracks_tags) {
                    if (Observation()->GetUnit(t) == nullptr) {
                        continue;
                    }
                    Point2D barracks_pos = Observation()->GetUnit(t)->pos;
                    if (Distance2D(barracks_pos, build_location) < (buildInfo->unit_radius) * 2) {
                        placeable = false;
                    }
                }
            }
            // verify that the build location has enough space around it to place a structure
            // same idea as above, keep rotating a vector around a location and check if there are any points around it that are not placable
            while (placeable) {
                double theta = Convert(inner_angle);

                cs = cos(theta);
                sn = sin(theta);

                px = ((check_location.x - build_location.x) * cs) - ((build_location.y - check_location.y) * sn) +
                     build_location.x;
                py = build_location.y - ((build_location.y - check_location.y) * cs) +
                     ((check_location.x - build_location.x) * sn);

                if (!Observation()->IsPlacable(Point2D(px, py)) || !Observation()->IsPathable(Point2D(px, py))) {
                    placeable = false;
                }

                inner_angle += 5;
                if (inner_angle == 360) {
                    break;
                }
            }
        }
    }

    buildInfo->angle = angle;
    buildInfo->previous_build = build_location;
    buildInfo->previous_radius = _radius;

    return build_location;

}


bool Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type, bool simult, std::string n) {
    const ObservationInterface *observation = Observation();

    // get a unit to build the structure
    const Unit *builder_unit = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self, IsUnit(unit_type));
    for (const auto &unit: units) {
        // do not interrupt an SCV that already has 2 orders
        if (unit->orders.size() >= 2) { continue; }

        for (const auto &order: unit->orders) {
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


    Point3D closest_mineral; // center of closest mineral field to starting location

    // init centers
    Point3D center1 = clusterCenters[0];
    Point3D center2 = clusterCenters[1];

    //init min distance found
    double min = Distance3D(center1, center2);

    double distance;
    // arbitrary distance from center of a cluster

    Point2D build_location;
    double radius;
    Point3D center_pos;
    switch (ability_type_for_structure) {
        case ABILITY_ID::BUILD_COMMANDCENTER:
            if (BuildMap.find("command") == end(BuildMap)) {
                BuildMap["command"] = BuildCommandInfo();
            }
            // find the center of the cluster that is closest to starting location
            for (const Point3D &center2: clusterCenters) {
                distance = Distance3D(center1, center2);
                if (distance < min && center2 != clusterCenters[0]) {
                    min = distance;
                    closest_mineral = center2;
                }
            }
            radius = BuildMap["command"].previous_radius;

            build_location = chooseNearbyBuildLocation(closest_mineral, radius, ability_type_for_structure, "command");

            Actions()->UnitCommand(builder_unit,
                                   ability_type_for_structure,
                                   build_location);

            break;
        case ABILITY_ID::BUILD_BARRACKS:

            // create a build info struct for this barracks if one doesn't already exist
            if (BuildMap.find(n) == end(BuildMap)) {
                BuildMap[n] = BuildBarracksInfo();
            }
            radius = BuildMap[n].previous_radius;
            if (Observation()->GetUnit(command_center_tags[0]) == nullptr) {
                break;
            }
            center_pos = Observation()->GetUnit(command_center_tags[0])->pos;
            build_location = chooseNearbyBuildLocation(center_pos, radius, ability_type_for_structure, n);

            Actions()->UnitCommand(builder_unit,
                                   ability_type_for_structure,
                                   build_location);
            break;
        default:
            if (Observation()->GetUnit(command_center_tags[0]) == nullptr) {
                break;
            }
            center_pos = Observation()->GetUnit(command_center_tags[0])->pos;
            float rx;
            float ry;
            int x;
            int y;
            bool placable = false;

            // keep choosing random location until a valid location is found
            while (!placable) {
                rx = GetRandomScalar();
                ry = GetRandomScalar();
                x = GetRandomInteger(-10, 10);
                y = GetRandomInteger(-10, 10);
                build_location = Point2D(center_pos.x + rx * x, center_pos.y + ry * y);
                while (!Observation()->IsPathable(build_location) || !Observation()->IsPlacable(build_location)) {
                    rx = GetRandomScalar();
                    ry = GetRandomScalar();
                    x = GetRandomInteger(-10, 10);
                    y = GetRandomInteger(-10, 10);
                    build_location = Point2D(center_pos.x + rx * x, center_pos.y + ry * y);
                }
                placable = true;

                // place buildings away from barracks
                for (const Tag &t: barracks_tags) {
                    if (Observation()->GetUnit(t) == nullptr) {
                        continue;
                    }
                    Point2D barracks_pos = Observation()->GetUnit(t)->pos;
                    if (Distance2D(barracks_pos, build_location) < 6) {
                        placable = false;
                        break;
                    }
                }
            }

            Actions()->UnitCommand(builder_unit,
                                   ability_type_for_structure,
                                   build_location);
            break;
    }

    return true;
}

bool Bot::CommandSCVs(int n, const Unit *target, ABILITY_ID ability) {
    if (CountUnitType(UNIT_TYPEID::TERRAN_SCV) >= n) {
        // gather n SCVs
        std::vector<const Unit *> scv_units;
        Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));

        // find SCVs
        for (const auto &unit: units) {
            // Skip the scouting SCV
            if (unit->tag == scouting_scv)
                continue;

            bool valid_scv = true;
            for (const auto &order: unit->orders) {
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


// finds the positions of all minerals where the base locations could be
void Bot::FindBaseLocations() {
    // vector storing the locations of all mineral fields
    std::vector<Point3D> mineralFields;

    // vector storing clusters of mineral fields
    std::vector<std::vector<Point3D>> clusters;

    const ObservationInterface *observation = Observation();

    for (auto &u: observation->GetUnits()) {
        if (isMineral(u)) {
            //std::cout << "mineral field at:" << u->pos.x << " " << u->pos.y << "facing: " << u->facing << std::endl;
            mineralFields.push_back(Point3D(u->pos.x, u->pos.y, u->pos.z));
        }
    }

    // get start location
    Point3D start_location = observation->GetStartLocation();
    double dist_threshold = 10;

    // find starting mineral cluster based on start location
    std::vector<Point3D> start_cluster;

    for (const Point3D &p: mineralFields) {
        if (sqrt(DistanceSquared3D(start_location, p)) <= dist_threshold) {
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
    for (const Point3D &p1: mineralFields) {
        // prevent finding duplicate clusters
        for (const std::vector<Point3D> &v: clusters) {
            if (!skip) {
                for (const Point3D &p: v) {
                    if (p == p1) {
                        skip = true;
                        break;
                    }
                }
            }
        }
        // find unique cluster based on distance threshold
        if (!skip) {
            cluster.push_back(p1);
            for (const Point3D &p2: mineralFields) {
                distance = sqrt(DistanceSquared3D(p1, p2));
                if (distance <= dist_threshold && p1 != p2) {
                    cluster.push_back(p2);
                }
            }
            clusters.push_back(cluster);
            ++numClusters;
            cluster.clear();
        }
        skip = false;
    }

    if (Observation()->GetUnit(command_center_tags[0]) != nullptr) {
        Point2D temp = Observation()->GetUnit(command_center_tags[0])->pos;
    }

    //std::cout << "distance between starting center and field:" << dist(computeClusterCenter(clusters[0]),temp)<< std::endl;

    Point3D center;
    for (const std::vector<Point3D> &p: clusters) {
        center = computeClusterCenter(p);
        //std::cout << "center: " << center.x << " " << center.y << " " << center.z << std::endl;
        clusterCenters.push_back(center);
    }
}


// checks if a unit is a mineral
bool Bot::isMineral(const Unit *u) {
    UNIT_TYPEID unit_type = u->unit_type;
    switch (unit_type) {
        case UNIT_TYPEID::NEUTRAL_MINERALFIELD         :
            return true;
        case UNIT_TYPEID::NEUTRAL_MINERALFIELD750      :
            return true;
        case UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD     :
            return true;
        case UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750  :
            return true;
        case UNIT_TYPEID::NEUTRAL_LABMINERALFIELD        :
            return true;
        case UNIT_TYPEID::NEUTRAL_LABMINERALFIELD750    :
            return true;
        default:
            return false;
    }
}

inline Units Bot::GetArmyUnits() {
    return Observation()->GetUnits(
            Unit::Alliance::Self, IsUnits({UNIT_TYPEID::TERRAN_MARINE, UNIT_TYPEID::TERRAN_MARAUDER}));
}

void Bot::AttackHandler() {
    const auto *observation = Observation();

    // Trigger an attack if either the time or number of army units thresholds are reached
    if ((float) observation->GetGameLoop() * SECONDS_PER_GAME_LOOP >= (float) config.attackTriggerTimeSeconds
        || GetArmyUnits().size() >= (size_t) config.attackTriggerArmyUnits) {
        units_should_attack = true;
    }

    if (!units_should_attack)
        return;

    const auto army_units = GetArmyUnits();

    // Get enemy units that are currently visible or were seen before
    auto enemy_units = observation->GetUnits(Unit::Alliance::Enemy, [](const auto &unit) {
        return unit.display_type == Unit::DisplayType::Visible || unit.display_type == Unit::DisplayType::Snapshot;
    });

    for (const auto *unit: army_units) {
        if (enemy_units.empty() && unit->orders.empty()) {
            CommandToSearchForEnemies(unit);
        } else {
            CommandToAttack(unit, enemy_units);
        }
    }
}

void Bot::CommandToAttack(const Unit *attacking_unit, const Units &enemy_units) {
    if (enemy_units.empty())
        return;

    // No need to command again if the unit is currently attacking
    if (!attacking_unit->orders.empty()) {
        const auto &current_order = attacking_unit->orders.front();
        if (current_order.ability_id == ABILITY_ID::ATTACK_ATTACK ||
            current_order.ability_id == ABILITY_ID::ATTACK) {
            return;
        }
    }

    // Attack the closest enemy unit
    const auto *unit_to_attack = *std::min_element(enemy_units.begin(), enemy_units.end(),
                                                   [attacking_unit](const auto &a, const auto &b) {
                                                       return DistanceSquared3D(attacking_unit->pos, a->pos) <
                                                              DistanceSquared3D(attacking_unit->pos, b->pos);
                                                   });

    if (have_stimpack && units_should_attack &&
        attacking_unit->health >= attacking_unit->health_max * config.stimpackMinHealth /* &&
        DistanceSquared3D(attacking_unit->pos, unit_to_attack->pos) <=
        config.stimpackMaxDistanceToEnemy * config.stimpackMaxDistanceToEnemy */) {
        // Apply Stimpack to the attacking unit
        if (attacking_unit->unit_type == UNIT_TYPEID::TERRAN_MARINE) {
            Actions()->UnitCommand(attacking_unit, ABILITY_ID::EFFECT_STIM_MARINE);
        } else if (attacking_unit->unit_type == UNIT_TYPEID::TERRAN_MARAUDER) {
            Actions()->UnitCommand(attacking_unit, ABILITY_ID::EFFECT_STIM_MARAUDER);
        }
    }

    if (unit_to_attack->unit_type == UNIT_TYPEID::ZERG_CHANGELINGMARINE ||
        unit_to_attack->unit_type == UNIT_TYPEID::ZERG_CHANGELINGMARINESHIELD) {
        // Special attack handling for Changeling, as they are not attackable in the same way as other units
        Actions()->UnitCommand(attacking_unit, ABILITY_ID::ATTACK, unit_to_attack);
    } else {
        Actions()->UnitCommand(attacking_unit, ABILITY_ID::ATTACK_ATTACK, unit_to_attack->pos);
    }
}

void Bot::CommandToSearchForEnemies(const Unit *unit) {
    // Mark enemy base as visited once an attacking unit is close enough to it
    if (enemy_base_location
        && !visited_enemy_base
        && DistanceSquared2D(unit->pos, *enemy_base_location) <= 5 * 5) {
        visited_enemy_base = true;
    }

    // If the enemy base has not been visited yet, try attacking it
    if (enemy_base_location && !visited_enemy_base) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK, *enemy_base_location);
        return;
    }

    if (!expansion_locations)
        return;

    // Otherwise, queue the unit to visit each expansion location in the order of distance
    auto sorted_expansions = *expansion_locations;
    std::sort(
            sorted_expansions.begin(), sorted_expansions.end(), [&](const auto &a, const auto &b) {
                return DistanceSquared3D(unit->pos, a) <
                       DistanceSquared3D(unit->pos, b);
            });
    for (const auto &loc: sorted_expansions) {
        Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, loc, true);
    }
}

void Bot::DefendHandler() {
    // Do not defend if we are already attacking
    if (units_should_attack)
        return;

    const auto *observation = Observation();
    const auto army_units = GetArmyUnits();

    // Get command center position
    const auto *cc = observation->GetUnit(command_center_tags[0]);
    if (!cc)
        return;

    // Get enemy units that are visible
    // Since we are defending (at our base), we only care about visible enemy units
    auto all_visible_enemies = observation->GetUnits(Unit::Alliance::Enemy, IsVisible());

    // Only defend against enemy units that are near our base
    std::vector<const Unit *> close_enemies;
    std::copy_if(all_visible_enemies.begin(), all_visible_enemies.end(), std::back_inserter(close_enemies),
                 [&](const auto &unit) {
                     return DistanceSquared3D(cc->pos, unit->pos) <= config.defendRadius * config.defendRadius;
                 });

    if (close_enemies.empty()) {
        // If there are no close enemies and we were just defending,
        // make our army units go back to the rally point (command center)
        if (just_defended) {
            just_defended = false;
            for (const auto *unit: army_units) {
                Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVE, cc->pos);
            }
        }
        return;
    }

    // Otherwise, defend against the close enemy units by attacking them
    just_defended = true;
    for (const auto *attacking_unit: army_units) {
        CommandToAttack(attacking_unit, close_enemies);
    }
}

void Bot::OnUnitEnterVision(const Unit *unit) {
    if (enemy_base_location != nullptr || !unit->is_building)
        return;

    const auto *observation = Observation();
    const auto enemy_start_locations = observation->GetGameInfo().enemy_start_locations;

    // Position of the enemy building that was seen
    const auto building_pos = Point2D(unit->pos);

    // Determine the closest enemy start location
    const auto closest_enemy_base_it = std::min_element(enemy_start_locations.begin(), enemy_start_locations.end(),
                                                        [&building_pos](const auto &a, const auto &b) {
                                                            return DistanceSquared2D(a, building_pos) <
                                                                   DistanceSquared2D(b, building_pos);
                                                        });

    // Set the closest enemy start location as the estimated enemy base location
    enemy_base_location = std::make_unique<Point2D>(*closest_enemy_base_it);

    // Order the scouting SCV to return to our command center and make it no longer the scouting SCV
    if (scouting_scv != 0 && !scouting_scv_returning) {
        const auto *scv = observation->GetUnit(scouting_scv);
        const auto *cc = observation->GetUnit(command_center_tags[0]);
        if (scv && cc) {
            Actions()->UnitCommand(scv, ABILITY_ID::MOVE_MOVE, cc->pos, false);
            scouting_scv_returning = true;
        }
    }

    std::cout << "DEBUG: Found enemy " << unit->unit_type.to_string() << " at " << building_pos.x << ','
              << building_pos.y << '\n';
    std::cout << "DEBUG: Estimating that enemy base location is at " << enemy_base_location->x << ','
              << enemy_base_location->y << '\n';
}

void Bot::SendScout() {
    const auto *observation = Observation();

    // Get an SCV to do the scouting
    const auto scvs = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_SCV));
    if (scvs.empty())
        return;

    const auto *scv = scvs.front();
    scouting_scv = scv->tag;
    std::cout << "DEBUG: Sending SCV " << scouting_scv << " to scout\n";

    // Get all possible enemy start locations
    auto enemy_start_locations = observation->GetGameInfo().enemy_start_locations;

    bool made_first_command = false;
    // Queue the scouting unit to visit each of the enemy start locations, picking the closest each time
    auto last_pos = Point2D(scv->pos);
    while (!enemy_start_locations.empty()) {
        const auto min_distance_it = std::min_element(enemy_start_locations.begin(),
                                                      enemy_start_locations.end(),
                                                      [&last_pos](const auto &a, const auto &b) {
                                                          return DistanceSquared2D(last_pos, a) <
                                                                 DistanceSquared2D(last_pos, b);
                                                      });
        const auto next_pos = *min_distance_it;
        enemy_start_locations.erase(min_distance_it);
        Actions()->UnitCommand(scv, ABILITY_ID::MOVE_MOVE, next_pos, made_first_command);
        last_pos = next_pos;
        made_first_command = true;
    }
}