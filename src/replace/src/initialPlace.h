#ifndef __REPLACE_INIT_PLACE__
#define __REPLACE_INIT_PLACE__

#include <Eigen/SparseCore>
#include "opendb/db.h"
#include <memory>

namespace replace {

class PlacerBase;
class Logger;
class InitialPlaceVars {
public:
  int maxIter;
  int minDiffLength;
  int maxSolverIter;
  int maxFanout;
  float netWeightScale;
  bool incrementalPlaceMode;

  InitialPlaceVars();
  void reset();
};

typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SMatrix;

class InitialPlace {
  public:
    InitialPlace();
    InitialPlace(InitialPlaceVars ipVars, 
        std::shared_ptr<PlacerBase> pb,
        std::shared_ptr<Logger> log);
    ~InitialPlace();

    void doBicgstabPlace();

  private:
    InitialPlaceVars ipVars_;
    std::shared_ptr<PlacerBase> pb_;
    std::shared_ptr<Logger> log_;

    // Solve two SparseMatrix equations here;
    //
    // find instLocVecX_
    // s.t. satisfies placeInstForceMatrixX_ * instLocVecX_ = fixedInstForceVecX_
    //
    // find instLocVecY_
    // s.t. satisfies placeInstForceMatrixY_ * instLocVecY_ = fixedInstForceVecY_
    //
    // instLocVecX_ : current/target instances' center X coordinates. 1-col vector.
    // instLocVecY_ : current/target instances' center Y coordinates. 1-col vector.
    //
    // fixedInstForceVecX_ : contains fixed instances' forces toward X coordi. 1-col vector.
    // fixedInstForceVecY_ : contains fixed instances' forces toward Y coordi. 1-col vector.
    //
    // placeInstForceMatrixX_ :
    //        SparseMatrix that contains connectivity forces on X // B2B model is used
    //
    // placeInstForceMatrixY_ :
    //        SparseMatrix that contains connectivity forces on Y // B2B model is used
    //
    // Used the interative BiCGSTAB solver to solve matrix eqs.

    Eigen::VectorXf instLocVecX_, fixedInstForceVecX_;
    Eigen::VectorXf instLocVecY_, fixedInstForceVecY_;
    SMatrix placeInstForceMatrixX_, placeInstForceMatrixY_;

    void placeInstsCenter();
    void setPlaceInstExtId();
    void updatePinInfo();
    void createSparseMatrix();
    void updateCoordi();
    void reset();
};

}
#endif
