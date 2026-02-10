// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

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

class MacroPlacer
{
 public:
  MacroPlacer(sta::dbNetwork* network,
              odb::dbDatabase* db,
              sta::dbSta* sta,
              utl::Logger* logger,
              par::PartitionMgr* tritonpart);
  ~MacroPlacer();

  bool place(int num_threads,
             int max_num_macro,
             int min_num_macro,
             int max_num_inst,
             int min_num_inst,
             float tolerance,
             int max_num_level,
             float coarsening_ratio,
             int large_net_threshold,
             int halo_width,
             int halo_height,
             odb::Rect global_fence,
             float area_weight,
             float outline_weight,
             float wirelength_weight,
             float guidance_weight,
             float fence_weight,
             float boundary_weight,
             float notch_weight,
             float macro_blockage_weight,
             float target_util,
             float min_ar,
             const char* report_directory,
             bool keep_clustering_data);

  void placeMacro(odb::dbInst* inst,
                  const float& x_origin,
                  const float& y_origin,
                  const odb::dbOrientType& orientation,
                  bool exact,
                  bool allow_overlap);
  std::vector<odb::dbInst*> findOverlappedMacros(odb::dbInst* macro);

  void setMacroPlacementFile(const std::string& file_name);
  void addGuidanceRegion(odb::dbInst* macro, odb::Rect region);
  void setMacroHalo(odb::dbInst* macro, int halo_width, int halo_height);

  void setDebug(std::unique_ptr<MplObserver>& graphics);
  void setDebugShowBundledNets(bool show_bundled_nets);
  void setDebugShowClustersIds(bool show_clusters_ids);
  void setDebugSkipSteps(bool skip_steps);
  void setDebugOnlyFinalResult(bool only_final_result);
  void setDebugTargetClusterId(int target_cluster_id);

 private:
  std::unique_ptr<HierRTLMP> hier_rtlmp_;
  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;

  std::map<odb::dbInst*, odb::Rect> guidance_regions_;
};

}  // namespace mpl
