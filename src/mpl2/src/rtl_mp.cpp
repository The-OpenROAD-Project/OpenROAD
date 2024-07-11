///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mpl2/rtl_mp.h"

#include "Mpl2Observer.h"
#include "hier_rtlmp.h"
#include "object.h"
#include "odb/db.h"

namespace mpl2 {
using odb::dbDatabase;
using std::string;
using utl::Logger;
using utl::MPL;

MacroPlacer2::MacroPlacer2() = default;
MacroPlacer2::~MacroPlacer2() = default;

class Snapper;

void MacroPlacer2::init(sta::dbNetwork* network,
                        odb::dbDatabase* db,
                        sta::dbSta* sta,
                        utl::Logger* logger,
                        par::PartitionMgr* tritonpart)
{
  hier_rtlmp_
      = std::make_unique<HierRTLMP>(network, db, sta, logger, tritonpart);
  logger_ = logger;
  db_ = db;
}

bool MacroPlacer2::place(const int num_threads,
                         const int max_num_macro,
                         const int min_num_macro,
                         const int max_num_inst,
                         const int min_num_inst,
                         const float tolerance,
                         const int max_num_level,
                         const float coarsening_ratio,
                         const int num_bundled_ios,
                         const int large_net_threshold,
                         const int signature_net_threshold,
                         const float halo_width,
                         const float halo_height,
                         const float fence_lx,
                         const float fence_ly,
                         const float fence_ux,
                         const float fence_uy,
                         const float area_weight,
                         const float outline_weight,
                         const float wirelength_weight,
                         const float guidance_weight,
                         const float fence_weight,
                         const float boundary_weight,
                         const float notch_weight,
                         const float macro_blockage_weight,
                         const float pin_access_th,
                         const float target_util,
                         const float target_dead_space,
                         const float min_ar,
                         const int snap_layer,
                         const bool bus_planning_on,
                         const char* report_directory)
{
  hier_rtlmp_->setClusterSize(
      max_num_macro, min_num_macro, max_num_inst, min_num_inst);
  hier_rtlmp_->setClusterSizeTolerance(tolerance);
  hier_rtlmp_->setMaxNumLevel(max_num_level);
  hier_rtlmp_->setClusterSizeRatioPerLevel(coarsening_ratio);
  hier_rtlmp_->setNumBundledIOsPerBoundary(num_bundled_ios);
  hier_rtlmp_->setLargeNetThreshold(large_net_threshold);
  hier_rtlmp_->setSignatureNetThreshold(signature_net_threshold);
  hier_rtlmp_->setHaloWidth(halo_width);
  hier_rtlmp_->setHaloHeight(halo_height);
  hier_rtlmp_->setGlobalFence(fence_lx, fence_ly, fence_ux, fence_uy);
  hier_rtlmp_->setAreaWeight(area_weight);
  hier_rtlmp_->setOutlineWeight(outline_weight);
  hier_rtlmp_->setWirelengthWeight(wirelength_weight);
  hier_rtlmp_->setGuidanceWeight(guidance_weight);
  hier_rtlmp_->setFenceWeight(fence_weight);
  hier_rtlmp_->setBoundaryWeight(boundary_weight);
  hier_rtlmp_->setNotchWeight(notch_weight);
  hier_rtlmp_->setMacroBlockageWeight(macro_blockage_weight);
  hier_rtlmp_->setPinAccessThreshold(pin_access_th);
  hier_rtlmp_->setTargetUtil(target_util);
  hier_rtlmp_->setTargetDeadSpace(target_dead_space);
  hier_rtlmp_->setMinAR(min_ar);
  hier_rtlmp_->setSnapLayer(snap_layer);
  hier_rtlmp_->setBusPlanningOn(bus_planning_on);
  hier_rtlmp_->setReportDirectory(report_directory);
  hier_rtlmp_->setNumThreads(num_threads);
  hier_rtlmp_->run();

  return true;
}

void MacroPlacer2::placeMacro(odb::dbInst* inst,
                              const float& x_origin,
                              const float& y_origin,
                              const odb::dbOrientType& orientation)
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
                   "Specified location results in illegal placement. Cannot "
                   "place macro outside of the core.");
  }

  // Orientation must be set before location so we don't end up flipping
  // and misplacing the macro.
  inst->setOrient(orientation);
  inst->setLocation(x1, y1);

  if (!orientation.isRightAngleRotation()) {
    Snapper snapper(inst);
    snapper.snapMacro();
  } else {
    logger_->warn(
        MPL,
        36,
        "Orientation {} specified for macro {} is a right angle rotation.",
        orientation.getString(),
        inst->getName());
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

void MacroPlacer2::setMacroPlacementFile(const std::string& file_name)
{
  hier_rtlmp_->setMacroPlacementFile(file_name);
}

void MacroPlacer2::setDebug(std::unique_ptr<Mpl2Observer>& graphics)
{
  hier_rtlmp_->setDebug(graphics);
}

void MacroPlacer2::setDebugShowBundledNets(bool show_bundled_nets)
{
  hier_rtlmp_->setDebugShowBundledNets(show_bundled_nets);
}

void MacroPlacer2::setDebugSkipSteps(bool skip_steps)
{
  hier_rtlmp_->setDebugSkipSteps(skip_steps);
}

void MacroPlacer2::setDebugOnlyFinalResult(bool only_final_result)
{
  hier_rtlmp_->setDebugOnlyFinalResult(only_final_result);
}

}  // namespace mpl2
