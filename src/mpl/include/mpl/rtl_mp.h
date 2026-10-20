// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {
class dbDatabase;
class dbInst;
class dbOrientType;
}  // namespace odb

namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta

namespace par {
class PartitionMgr;
}

namespace mpl {

class HierRTLMP;
class MplObserver;

// Every parameter accepted by the `rtl_macro_placer` command.
struct MacroPlacerOptions
{
  int max_num_macro{0};
  int min_num_macro{0};
  int max_num_inst{0};
  int min_num_inst{0};
  float tolerance{0.1f};
  int max_num_level{2};
  float coarsening_ratio{10.0f};
  int large_net_threshold{50};

  odb::Rect global_fence;

  float area_weight{0.1f};
  float outline_weight{100.0f};
  float wirelength_weight{100.0f};
  float guidance_weight{10.0f};
  float fence_weight{10.0f};
  float boundary_weight{50.0f};
  float notch_weight{50.0f};
  float soft_blockage_weight{10.0f};

  float target_util{0.25f};
  float min_ar{0.33f};

  std::string report_directory{"hier_rtlmp"};
  std::string macro_placement_file;

  bool keep_clustering_data{false};
  bool pin_aware_channels{false};
};

// Every parameter accepted by the `mpl_debug` command.
struct MacroPlacerDebugOptions
{
  MacroPlacerDebugOptions() = default;
  ~MacroPlacerDebugOptions() = default;
  MacroPlacerDebugOptions(MacroPlacerDebugOptions&&) noexcept = default;
  MacroPlacerDebugOptions& operator=(MacroPlacerDebugOptions&&) noexcept
      = default;

  // configureDebug() moves this into place before applying the rest of
  // these fields to it, so a default-constructed instance (graphics ==
  // nullptr) is not valid to pass to configureDebug().
  std::unique_ptr<MplObserver> graphics;

  bool show_bundled_nets{false};
  bool show_clusters_ids{false};
  bool skip_steps{false};
  bool only_final_result{false};
  int target_cluster_id{-1};
};

class MacroPlacer
{
 public:
  MacroPlacer(odb::dbDatabase* db,
              sta::dbSta* sta,
              utl::Logger* logger,
              par::PartitionMgr* tritonpart);
  ~MacroPlacer();

  bool place(int threads, const MacroPlacerOptions& options = {});

  void blockMacroChannels();

  void placeMacro(odb::dbInst* inst,
                  const float& x_origin,
                  const float& y_origin,
                  const odb::dbOrientType& orientation,
                  bool exact,
                  bool allow_overlap);
  std::vector<odb::dbInst*> findOverlappedMacros(odb::dbInst* macro);

  void addGuidanceRegion(odb::dbInst* macro, odb::Rect region);
  void setMinChannelSize(int width, int height);

  void configureDebug(MacroPlacerDebugOptions options);

 private:
  std::unique_ptr<HierRTLMP> hier_rtlmp_;
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;

  odb::PtrMap<odb::dbInst, odb::Rect> guidance_regions_;
};

}  // namespace mpl
