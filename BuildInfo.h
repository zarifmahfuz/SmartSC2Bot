#include <sc2api/sc2_api.h>
#include <sc2api/sc2_unit_filters.h>
using namespace sc2;
struct BuildInfo{
    public:
        Point3D previous_build;
        int previous_radius;
        int unit_radius;
        int iter;
        double angle;
        BuildInfo(){
            previous_build = Point3D(0,0,0);
            iter = 0;
            angle = 10;
        }
        BuildInfo(const BuildInfo &rhs){
            previous_build = rhs.previous_build;
            previous_radius = rhs.previous_radius;
            iter = rhs.iter;
            angle =  rhs.angle;
            unit_radius = rhs.unit_radius;
        }
        BuildInfo &operator=(const BuildInfo &rhs){
            previous_build = rhs.previous_build;
            previous_radius = rhs.previous_radius;
            iter = rhs.iter;
            angle =  rhs.angle;
            unit_radius = rhs.unit_radius;
            return *this;
        }

};