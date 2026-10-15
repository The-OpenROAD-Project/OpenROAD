// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "grt/GlobalRouteOptions.h"
#include "rsz/RepairDesignOptions.hh"
#include "utl/Validate.h"

namespace odb {
class dbBTerm;
class dbDatabase;
class dbInst;

}  // namespace odb
namespace sta {
class dbSta;
}

namespace ppl {
class IOPlacer;
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
class ClockBase;

class InitialPlace;
class NesterovPlace;

using Cluster = std::vector<odb::dbInst*>;
using Clusters = std::vector<Cluster>;

// The options of global_placement, declared once for C++, Python and Tcl.
// Each struct publishes a schema (utl/Validate.h) that carries the help,
// the constraints and the message ids; see replace.cpp.

struct InitialPlaceOptions
{
  // Iterations of the quadratic (BiCGSTAB) initial placement; 0 skips it.
  int max_iter = 20;
  int min_diff_length = 1500;
  int max_solver_iter = 100;
  // Nets with more pins are ignored by the initial placement.
  int max_fanout = 200;
  float net_weight_scale = 800;
  // Start from the core center even when the instances have locations.
  bool force_center = false;
  int perturbation_seed = 1;
  // Random perturbation of the initial locations in DBU; -1 for the
  // internal default.
  float perturbation_dist = -1.0f;

  static const utl::Schema<InitialPlaceOptions>& schema();
};

struct NesterovOptions
{
  int max_iter = 5000;
  // Overflow at which the placement stops.
  float target_overflow = 0.1;
  float init_density_penalty = 0.00008;
  float init_wirelength_coef = 0.25;
  float min_phi_coef = 0.95;
  float max_phi_coef = 1.05;
  // Scale the density-penalty controller measures a wirelength change
  // against, in DBU. The controller eases the penalty ramp in proportion to
  // (delta HPWL / reference_hpwl), so a fixed value only suits designs whose
  // HPWL happens to be near it. 0 derives it from the design's own HPWL.
  float reference_hpwl = 0;
  // Keep snapshots and revert to the last one when the placement diverges.
  bool revert_if_diverge = true;

  static const utl::Schema<NesterovOptions>& schema();
};

struct TimingOptions
{
  bool enabled = false;
  // Overflows at which the nets are reweighted from the resizer's slacks.
  std::vector<int> reweight_overflows{64, 20};
  float net_weight_max = 5;
  float nets_percentage = 10;
  // Below this overflow the resizer's changes are kept (non-virtual repair).
  float keep_resize_below_overflow = 1.0;
  bool repair_timing = false;
  float repair_tns_end_percent = 1.0;
  // The repair_design run that finds the slacks.  Its max_wire_length of 0
  // is the delay-optimal length.
  rsz::RepairDesignOptions repair_design;

  static const utl::Schema<TimingOptions>& schema();
};

struct VirtualCtsOptions
{
  // Model the clock insertion delay with a lightweight clock tree before
  // each timing-driven iteration.
  bool enabled = false;
  // Maximum clock insertion delay as a fraction of the clock period.
  // The MST leaf farthest from the virtual clock root gets this delay;
  // all others are scaled proportionally.
  float max_skew_fraction = 0.10f;

  static const utl::Schema<VirtualCtsOptions>& schema();
};

struct RoutabilityOptions
{
  bool enabled = false;
  // Estimate congestion with RUDY; otherwise run the global router.
  bool use_rudy = true;
  float check_overflow = 0.3;
  float snapshot_overflow = 0.6;
  float max_density = 0.99;
  float target_rc_metric = 1.01;
  float inflation_ratio_coef = 2;
  float max_inflation_ratio = 3;
  float min_congestion_for_inflation = 0.95;
  // Ceiling on the artificial area routability may add, as a fraction of the
  // movable area it started from. Inflation is the only thing in the loop that
  // grows the design, and past some point the density penalty is handed more
  // area than the floorplan can absorb.
  float max_inflation_total = 0.30;
  // Wirelength weight applied to the nets that put the most wire into the
  // congested tiles, and the share of those nets that gets weighted. A net
  // deposits demand in proportion to its own length, so pulling the worst
  // offenders shorter takes demand out of the region they cross - unlike
  // inflating cells, which adds wirelength everywhere.
  //
  // Off by default at 1.0. The weight multiplies onto the timing weight, so a
  // net can be pulled for congestion and criticality at once, and shortening a
  // net packs its cells tighter into the region that was already congested.
  // Measured on the flow tests it costs setup slack and clock skew without
  // buying congestion back, so it stays opt-in until it earns its default.
  float net_weight_max = 1.0;
  float congested_nets_percentage = 1.0;
  // RC metric coefficients k1..k4.
  std::array<float, 4> rc_coefficients{1.0, 1.0, 0.0, 0.0};
  // The global route run when not using RUDY, and for the congestion
  // heatmap.  Placement needs a quick congestion estimate, not a legal
  // route, hence no rip-up iterations and congestion allowed.
  grt::GlobalRouteOptions global_route{.allow_congestion = true,
                                       .congestion_iterations = 0,
                                       .critical_nets_percentage = 0.0f,
                                       .save_guides = false};

  static const utl::Schema<RoutabilityOptions>& schema();
};

struct PlaceOptions
{
  // Target density; 0.7 is 70%.
  float density = 0.7;
  // Use the minimum feasible target density instead of density.
  bool uniform_density = false;
  // Bin grid size; 0 lets the placer choose.
  int bin_grid_count_x = 0;
  int bin_grid_count_y = 0;
  // Padding in sites.
  int pad_left = 0;
  int pad_right = 0;
  // Ignore the IO pins when computing wirelength.
  bool skip_io = false;
  // Place the movable IO pins concurrently with the cells.
  bool place_ios = false;
  // Grow the area of pin-dense instances.
  bool pin_density_adjust = true;
  // Run the global router after placement, for the routing congestion
  // heatmap.
  bool routing_congestion = false;

  InitialPlaceOptions initial;
  NesterovOptions nesterov;
  TimingOptions timing;
  VirtualCtsOptions virtual_cts;
  RoutabilityOptions routability;

  // Ignores the IO pins, and with them the initial placement and the
  // timing and routability driven modes that need them placed.
  void skipIo();

  static const utl::Schema<PlaceOptions>& schema();
};

class Replace
{
 public:
  // Create a replace object with no graphics.
  Replace(odb::dbDatabase* odb,
          sta::dbSta* sta,
          rsz::Resizer* resizer,
          grt::GlobalRouter* router,
          ppl::IOPlacer* pin_placer,
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

  void runMBFF(int max_sz,
               float alpha,
               float beta,
               int threads,
               int num_paths,
               float clock_power_weight);

  void addPlacementCluster(const Cluster& cluster);

  // Query for uniform density value
  float getUniformTargetDensity(const PlaceOptions& options, int threads);
  float estimateTargetDensity(const PlaceOptions& options, int threads);

  void setDebug(int pause_iterations,
                int update_iterations,
                bool draw_bins,
                bool initial,
                odb::dbInst* inst,
                odb::dbBTerm* pin,
                int start_iter,
                int start_rudy,
                int rudy_stride,
                bool generate_images,
                const std::string& images_path);

 private:
  bool initNesterovPlace(const PlaceOptions& options,
                         int threads,
                         bool check_density);
  void checkHasCoreRows();
  void checkPlaceIosSupported(const PlaceOptions& options);
  void reportHpwlMetric();
  // The top-level (unfenced/full-die) region is always nbVec_[0].
  NesterovBase* getTopLevelNB() const;
  void initIoPinPlace();

  odb::dbDatabase* db_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  rsz::Resizer* rs_ = nullptr;
  grt::GlobalRouter* fr_ = nullptr;
  ppl::IOPlacer* pin_placer_ = nullptr;
  utl::Logger* log_ = nullptr;

  std::unique_ptr<AbstractGraphics> graphics_;

  std::shared_ptr<PlacerBaseCommon> pbc_;
  std::shared_ptr<NesterovBaseCommon> nbc_;
  std::vector<std::shared_ptr<PlacerBase>> pbVec_;
  std::vector<std::shared_ptr<NesterovBase>> nbVec_;
  std::shared_ptr<RouteBase> rb_;
  std::shared_ptr<TimingBase> tb_;
  std::shared_ptr<ClockBase> cb_;

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
  odb::dbBTerm* gui_debug_pin_ = nullptr;
  int gui_debug_start_iter_ = 0;
  int gui_debug_rudy_start_ = 0;
  int gui_debug_rudy_stride_ = 0;
  bool gui_debug_generate_images_ = false;
  std::string gui_debug_images_path_ = "REPORTS_DIR";
};

}  // namespace gpl
