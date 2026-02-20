// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "mpl/rtl_mp.h"

#include <memory>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "hier_rtlmp.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using std::string;
using utl::MPL;

class Snapper;

MacroPlacer::MacroPlacer(sta::dbNetwork* network,
                         odb::dbDatabase* db,
                         sta::dbSta* sta,
                         utl::Logger* logger,
                         par::PartitionMgr* tritonpart)
{
  hier_rtlmp_ = std::make_unique<HierRTLMP>(network, db, logger, tritonpart);
  logger_ = logger;
  db_ = db;
}

MacroPlacer::~MacroPlacer() = default;

bool MacroPlacer::place(const int num_threads,
                        const int max_num_macro,
                        const int min_num_macro,
                        const int max_num_inst,
                        const int min_num_inst,
                        const float tolerance,
                        const int max_num_level,
                        const float coarsening_ratio,
                        const int large_net_threshold,
                        const int halo_width,
                        const int halo_height,
                        const odb::Rect global_fence,
                        const float area_weight,
                        const float outline_weight,
                        const float wirelength_weight,
                        const float guidance_weight,
                        const float fence_weight,
                        const float boundary_weight,
                        const float notch_weight,
                        const float macro_blockage_weight,
                        const float target_util,
                        const float min_ar,
                        const char* report_directory,
                        const bool keep_clustering_data)
{
  hier_rtlmp_->init();
  hier_rtlmp_->setClusterSize(
      max_num_macro, min_num_macro, max_num_inst, min_num_inst);
  hier_rtlmp_->setClusterSizeTolerance(tolerance);
  hier_rtlmp_->setMaxNumLevel(max_num_level);
  hier_rtlmp_->setClusterSizeRatioPerLevel(coarsening_ratio);
  hier_rtlmp_->setLargeNetThreshold(large_net_threshold);
  hier_rtlmp_->setDefaultHalo(halo_width, halo_height);
  hier_rtlmp_->setGlobalFence(global_fence);
  hier_rtlmp_->setAreaWeight(area_weight);
  hier_rtlmp_->setOutlineWeight(outline_weight);
  hier_rtlmp_->setWirelengthWeight(wirelength_weight);
  hier_rtlmp_->setGuidanceWeight(guidance_weight);
  hier_rtlmp_->setFenceWeight(fence_weight);
  hier_rtlmp_->setBoundaryWeight(boundary_weight);
  hier_rtlmp_->setNotchWeight(notch_weight);
  hier_rtlmp_->setMacroBlockageWeight(macro_blockage_weight);
  hier_rtlmp_->setTargetUtil(target_util);
  hier_rtlmp_->setMinAR(min_ar);
  hier_rtlmp_->setReportDirectory(report_directory);
  hier_rtlmp_->setNumThreads(num_threads);
  hier_rtlmp_->setKeepClusteringData(keep_clustering_data);
  hier_rtlmp_->setGuidanceRegions(guidance_regions_);

  hier_rtlmp_->run();

  return true;
}

void MacroPlacer::placeMacro(odb::dbInst* inst,
                             const float& x_origin,
                             const float& y_origin,
                             const odb::dbOrientType& orientation,
                             const bool exact,
                             const bool allow_overlap)
{
  odb::dbBlock* block = inst->getBlock();

  const int x1 = block->micronsToDbu(x_origin);
  const int y1 = block->micronsToDbu(y_origin);
  const int x2 = x1 + inst->getBBox()->getDX();
  const int y2 = y1 + inst->getBBox()->getDY();

  odb::Rect macro_new_bbox(x1, y1, x2, y2);
  odb::Rect core_area = inst->getBlock()->getCoreArea();

  if (!core_area.contains(macro_new_bbox)) {
    logger_->error(MPL,
                   34,
                   "Cannot place {} at ({}, {}) ({}, {}), outside of the core "
                   "({}, {}) ({}, {}).",
                   inst->getName(),
                   block->dbuToMicrons(macro_new_bbox.xMin()),
                   block->dbuToMicrons(macro_new_bbox.yMin()),
                   block->dbuToMicrons(macro_new_bbox.xMax()),
                   block->dbuToMicrons(macro_new_bbox.yMax()),
                   block->dbuToMicrons(core_area.xMin()),
                   block->dbuToMicrons(core_area.yMin()),
                   block->dbuToMicrons(core_area.xMax()),
                   block->dbuToMicrons(core_area.yMax()));
  }

  // Orientation must be set before location so we don't end up flipping
  // and misplacing the macro.
  inst->setOrient(orientation);
  inst->setLocation(x1, y1);

  if (orientation.isRightAngleRotation()) {
    logger_->warn(MPL,
                  36,
                  "Orientation {} specified for macro {} is a right angle "
                  "rotation. Snapping is not possible.",
                  orientation.getString(),
                  inst->getName());
  } else if (!exact) {
    Snapper snapper(logger_, inst);
    snapper.snapMacro();
  }

  if (!allow_overlap) {
    std::vector<odb::dbInst*> overlapped_macros = findOverlappedMacros(inst);
    if (!overlapped_macros.empty()) {
      std::string overlapped_macros_names;
      for (odb::dbInst* overlapped_macro : overlapped_macros) {
        overlapped_macros_names
            += fmt::format(" {}", overlapped_macro->getName());
      }

      logger_->error(MPL,
                     41,
                     "Couldn't place {}. Found overlap with other macros:{}.",
                     inst->getName(),
                     overlapped_macros_names);
    }
  }

  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);

  logger_->info(MPL,
                35,
                "Macro {} placed. Bounding box ({:.3f}um, {:.3f}um), "
                "({:.3f}um, {:.3f}um). Orientation {}",
                inst->getName(),
                block->dbuToMicrons(inst->getBBox()->xMin()),
                block->dbuToMicrons(inst->getBBox()->yMin()),
                block->dbuToMicrons(inst->getBBox()->xMax()),
                block->dbuToMicrons(inst->getBBox()->yMax()),
                orientation.getString());
}

std::vector<odb::dbInst*> MacroPlacer::findOverlappedMacros(odb::dbInst* macro)
{
  std::vector<odb::dbInst*> overlapped_macros;
  odb::dbBlock* block = macro->getBlock();
  const odb::Rect& source_macro_bbox = macro->getBBox()->getBox();

  for (odb::dbInst* inst : block->getInsts()) {
    if (!inst->isBlock() || !inst->isPlaced()) {
      continue;
    }

    const odb::Rect& target_macro_bbox = inst->getBBox()->getBox();
    if (source_macro_bbox.overlaps(target_macro_bbox)) {
      overlapped_macros.push_back(inst);
    }
  }

  return overlapped_macros;
}

void MacroPlacer::addGuidanceRegion(odb::dbInst* macro, odb::Rect region)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  const odb::Rect& core = block->getCoreArea();

  if (!core.contains(region)) {
    logger_->error(MPL,
                   42,
                   "Specified guidance region ({}, {}) ({}, {}) for the macro "
                   "{} is outside of the core ({}, {}) ({}, {}).",
                   region.xMin(),
                   region.yMin(),
                   region.xMax(),
                   region.yMax(),
                   macro->getName(),
                   block->dbuToMicrons(core.xMin()),
                   block->dbuToMicrons(core.yMin()),
                   block->dbuToMicrons(core.xMax()),
                   block->dbuToMicrons(core.yMax()));
  }

  if (guidance_regions_.find(macro) != guidance_regions_.end()) {
    logger_->warn(
        MPL, 44, "Overwriting guidance region for macro {}", macro->getName());
  }

  guidance_regions_[macro] = region;
}

void MacroPlacer::setMacroHalo(odb::dbInst* macro,
                               int halo_width,
                               int halo_height)
{
  hier_rtlmp_->setMacroHalo(macro, halo_width, halo_height);
}

void MacroPlacer::setMacroPlacementFile(const std::string& file_name)
{
  hier_rtlmp_->setMacroPlacementFile(file_name);
}

void MacroPlacer::setDebug(std::unique_ptr<MplObserver>& graphics)
{
  hier_rtlmp_->setDebug(graphics);
}

void MacroPlacer::setDebugShowBundledNets(bool show_bundled_nets)
{
  hier_rtlmp_->setDebugShowBundledNets(show_bundled_nets);
}
void MacroPlacer::setDebugShowClustersIds(bool show_clusters_ids)
{
  hier_rtlmp_->setDebugShowClustersIds(show_clusters_ids);
}

void MacroPlacer::setDebugSkipSteps(bool skip_steps)
{
  hier_rtlmp_->setDebugSkipSteps(skip_steps);
}

void MacroPlacer::setDebugOnlyFinalResult(bool only_final_result)
{
  hier_rtlmp_->setDebugOnlyFinalResult(only_final_result);
}

void MacroPlacer::setDebugTargetClusterId(const int target_cluster_id)
{
  hier_rtlmp_->setDebugTargetClusterId(target_cluster_id);
}

}  // namespace mpl
