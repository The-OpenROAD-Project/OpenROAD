// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace odb {
class dbDatabase;
class dbInst;

}  // namespace odb
namespace sta {
class dbSta;
}

namespace grt {
class GlobalRouter;
}

namespace rsz {
class Resizer;
}

namespace utl {
class Logger;
}

namespace gpl {

class AbstractGraphics;
class PlacerBaseCommon;
class PlacerBase;
class NesterovBaseCommon;
class NesterovBase;
class RouteBase;
class TimingBase;

class InitialPlace;
class NesterovPlace;

using Cluster = std::vector<odb::dbInst*>;
using Clusters = std::vector<Cluster>;

struct PlaceOptions
{
  int initialPlaceMaxIter = 20;
  int initialPlaceMinDiffLength = 1500;
  int initialPlaceMaxSolverIter = 100;
  int initialPlaceMaxFanout = 200;
  float initialPlaceNetWeightScale = 800;

  bool skipIoMode = false;
  bool forceCenterInitialPlace = false;
  bool timingDrivenMode = false;
  bool routabilityDrivenMode = false;
  bool uniformTargetDensityMode = false;
  std::vector<int> timingNetWeightOverflows{64, 20};
  float timingNetWeightMax = 5;
  float overflow = 0.1;
  int nesterovPlaceMaxIter = 5000;
  // timing driven check overflow to keep resizer changes (non-virtual resizer)
  float keepResizeBelowOverflow = 1.0;
  bool routabilityUseRudy = true;
  bool disableRevertIfDiverge = false;
  bool disablePinDensityAdjust = false;
  bool enable_routing_congestion = false;
  float minPhiCoef = 0.95;
  float maxPhiCoef = 1.05;
  float initDensityPenaltyFactor = 0.00008;
  float initWireLengthCoef = 0.25;
  float referenceHpwl = 446000000;
  int binGridCntX = 0;
  int binGridCntY = 0;
  float density = 0.7;

  float routabilityCheckOverflow = 0.3;
  float routabilitySnapshotOverflow = 0.6;
  float routabilityMaxDensity = 0.99;
  float routabilityTargetRcMetric = 1.01;
  float routabilityInflationRatioCoef = 2;
  float routabilityMaxInflationRatio = 3;

  // routability RC metric coefficients
  float routabilityRcK1 = 1.0;
  float routabilityRcK2 = 1.0;
  float routabilityRcK3 = 0.0;
  float routabilityRcK4 = 0.0;

  // OpenDB should have these values.
  int padLeft = 0;
  int padRight = 0;

  void skipIo();
  void validate(utl::Logger* log);
};

class Replace
{
 public:
  // Create a replace object with no graphics.
  Replace(odb::dbDatabase* odb,
          sta::dbSta* sta,
          rsz::Resizer* resizer,
          grt::GlobalRouter* router,
          utl::Logger* logger);

  ~Replace();

  // Use the following class as a template for graphics interface.
  //
  // Note: no ownership is transfered as the object will create a new
  // graphics object of the same class.
  void setGraphicsInterface(const gpl::AbstractGraphics& graphics);

  void reset();

  void doIncrementalPlace(int threads, const PlaceOptions& options = {});
  void doPlace(int threads, const PlaceOptions& options = {});
  void doInitialPlace(int threads, const PlaceOptions& options = {});
  int doNesterovPlace(int threads,
                      const PlaceOptions& options = {},
                      int start_iter = 0);

  void runMBFF(int max_sz, float alpha, float beta, int threads, int num_paths);

  void addPlacementCluster(const Cluster& cluster);

  // Query for uniform density value
  float getUniformTargetDensity(const PlaceOptions& options, int threads);

  void setDebug(int pause_iterations,
                int update_iterations,
                bool draw_bins,
                bool initial,
                odb::dbInst* inst,
                int start_iter,
                int start_rudy,
                int rudy_stride,
                bool generate_images,
                const std::string& images_path);

 private:
  bool initNesterovPlace(const PlaceOptions& options, int threads);
  void checkHasCoreRows();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* rs_ = nullptr;
  grt::GlobalRouter* fr_ = nullptr;
  utl::Logger* log_ = nullptr;

  std::unique_ptr<AbstractGraphics> graphics_;

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;

  std::unique_ptr<InitialPlace> ip_;
  std::unique_ptr<NesterovPlace> np_;

  int total_placeable_insts_ = 0;

  Clusters clusters_;

  bool gui_debug_ = false;
  int gui_debug_pause_iterations_ = 10;
  int gui_debug_update_iterations_ = 10;
  int gui_debug_draw_bins_ = false;
  int gui_debug_initial_ = false;
  odb::dbInst* gui_debug_inst_ = nullptr;
  int gui_debug_start_iter_ = 0;
  int gui_debug_rudy_start_ = 0;
  int gui_debug_rudy_stride_ = 0;
  bool gui_debug_generate_images_ = false;
  std::string gui_debug_images_path_ = "REPORTS_DIR";
};

}  // namespace gpl
