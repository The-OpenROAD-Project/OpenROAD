#ifndef __REPLACE_HEADER__
#define __REPLACE_HEADER__

#include <memory>

namespace odb {
  class dbDatabase;
}
namespace sta {
  class dbSta;
}
namespace FastRoute{
  class FastRouteKernel;
}

namespace replace {

class PlacerBase;
class NesterovBase;
class RouteBase;

class InitialPlace;
class NesterovPlace;

class Logger;

class Replace
{
  public:
    Replace();
    ~Replace();

    void init();
    void reset();

    void setDb(odb::dbDatabase* odb);
    void setSta(sta::dbSta* dbSta);
    void setFastRoute(FastRoute::FastRouteKernel* fr);

    void doInitialPlace();
    void doNesterovPlace();

    // Initial Place param settings
    void setInitialPlaceMaxIter(int iter);
    void setInitialPlaceMinDiffLength(int length);
    void setInitialPlaceMaxSolverIter(int iter);
    void setInitialPlaceMaxFanout(int fanout);
    void setInitialPlaceNetWeightScale(float scale);

    void setNesterovPlaceMaxIter(int iter);

    void setBinGridCntX(int binGridCntX);
    void setBinGridCntY(int binGridCntY);

    void setTargetDensity(float density);
    void setTargetOverflow(float overflow);
    void setInitDensityPenalityFactor(float penaltyFactor);
    void setInitWireLengthCoef(float coef);
    void setMinPhiCoef(float minPhiCoef);
    void setMaxPhiCoef(float maxPhiCoef);

    // HPWL: half-parameter wire length.
    void setReferenceHpwl(float deltaHpwl);

    void setIncrementalPlaceMode(bool mode);
    void setVerboseLevel(int verbose);
    
    // temp funcs; OpenDB should have these values. 
    void setPadLeft(int padding);
    void setPadRight(int padding);

    void setTimingDrivenMode(bool mode);

    void setRoutabilityDrivenMode(bool mode);
    void setRoutabilityCheckOverflow(float overflow);
    void setRoutabilityMaxDensity(float density);

    void setRoutabilityMaxBloatIter(int iter);
    void setRoutabilityMaxInflationIter(int iter);

    void setRoutabilityTargetRcMetric(float rc);
    void setRoutabilityInflationRatioCoef(float ratio);
    void setRoutabilityPitchScale(float scale);
    void setRoutabilityMaxInflationRatio(float ratio);

    void setRoutabilityRcCoefficients(float k1, float k2, float k3, float k4);

  private:
    odb::dbDatabase* db_;
    sta::dbSta* sta_;
    FastRoute::FastRouteKernel* fr_;

    std::shared_ptr<PlacerBase> pb_;
    std::shared_ptr<NesterovBase> nb_;
    std::shared_ptr<RouteBase> rb_;

    std::unique_ptr<InitialPlace> ip_;
    std::unique_ptr<NesterovPlace> np_;

    std::shared_ptr<replace::Logger> log_;

    int initialPlaceMaxIter_;
    int initialPlaceMinDiffLength_;
    int initialPlaceMaxSolverIter_;
    int initialPlaceMaxFanout_;
    float initialPlaceNetWeightScale_;

    int nesterovPlaceMaxIter_;
    int binGridCntX_;
    int binGridCntY_;
    float overflow_;
    float density_;
    float initDensityPenalityFactor_;
    float initWireLengthCoef_;
    float minPhiCoef_;
    float maxPhiCoef_;
    float referenceHpwl_;
    float routabilityCheckOverflow_;
    float routabilityMaxDensity_;
    float routabilityTargetRcMetric_;
    float routabilityInflationRatioCoef_;
    float routabilityPitchScale_;
    float routabilityMaxInflationRatio_;

    // routability RC metric coefficients
    float routabilityRcK1_, routabilityRcK2_, routabilityRcK3_, routabilityRcK4_;

    int routabilityMaxBloatIter_;
    int routabilityMaxInflationIter_;

    bool timingDrivenMode_;
    bool routabilityDrivenMode_;
    bool incrementalPlaceMode_;
   
    // temp variable; OpenDB should have these values. 
    int padLeft_;
    int padRight_;

    int verbose_;
};
}

#endif
