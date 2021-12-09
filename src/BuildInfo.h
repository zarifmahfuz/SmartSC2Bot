#include <sc2api/sc2_api.h>
#include <sc2api/sc2_unit_filters.h>

using namespace sc2;

// struct used to store info related to choosing a placement location for a building
struct BuildInfo {

public:
    // the previously chosen location
    Point2D previous_build;

    // the distance between the previosly chosen placement location and a chosen center point
    int previous_radius;

    // the dfault radius of how far a unit should be placed from a chosen center point
    int default_radius;

    // radius of the unit to be built
    int unit_radius;

    // angle to rotate a vector by around a center when computing a placement location
    double angle;

    // Constructor
    BuildInfo() {
        previous_build = Point2D(0, 0);
        angle = 5;
    }

    // Copy constructor
    BuildInfo(const BuildInfo &rhs) {
        previous_build = rhs.previous_build;
        previous_radius = rhs.previous_radius;
        angle = rhs.angle;
        unit_radius = rhs.unit_radius;
        default_radius = rhs.default_radius;
    }

    // Assignment constructor
    BuildInfo &operator=(const BuildInfo &rhs) {
        previous_build = rhs.previous_build;
        previous_radius = rhs.previous_radius;
        angle = rhs.angle;
        unit_radius = rhs.unit_radius;
        default_radius = rhs.default_radius;
        return *this;
    }

};