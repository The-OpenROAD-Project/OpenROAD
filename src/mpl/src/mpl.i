// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "mpl/rtl_mp.h"
#include "MplObserver.h"
#include "graphics.h"
#include "odb/db.h"

namespace ord {
// Defined in OpenRoad.i
mpl::MacroPlacer*
getMacroPlacer();
utl::Logger* getLogger();
}

using utl::MPL;
using ord::getMacroPlacer;
%}

%include "../../Exception.i"
%include <std_string.i>

%inline %{

namespace mpl {

bool rtl_macro_placer_cmd(const int max_num_macro,
                          const int min_num_macro,
                          const int max_num_inst,
                          const int min_num_inst,
                          const float tolerance,
                          const int max_num_level,
                          const float coarsening_ratio,
                          const int large_net_threshold,
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
                          const float target_util,
                          const float min_ar,
                          const char* report_directory,
                          const bool keep_clustering_data) {

  auto macro_placer = getMacroPlacer();
  const int num_threads = ord::OpenRoad::openRoad()->getThreadCount();
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  odb::Rect global_fence = odb::Rect(block->micronsToDbu(fence_lx),
                                    block->micronsToDbu(fence_ly),
                                    block->micronsToDbu(fence_ux),
                                    block->micronsToDbu(fence_uy));
  return macro_placer->place(num_threads,
                             max_num_macro,
                             min_num_macro,
                             max_num_inst,
                             min_num_inst,
                             tolerance,
                             max_num_level,
                             coarsening_ratio,
                             large_net_threshold,
                             block->micronsToDbu(halo_width),
                             block->micronsToDbu(halo_height),
                             global_fence,
                             area_weight,
                             outline_weight,
                             wirelength_weight,
                             guidance_weight,
                             fence_weight,
                             boundary_weight,
                             notch_weight,
                             macro_blockage_weight,
                             target_util,
                             min_ar,
                             report_directory,
                             keep_clustering_data);
}

void set_debug_cmd(odb::dbBlock* block,
                   bool coarse,
                   bool fine,
                   bool show_bundled_nets,
                   bool show_clusters_ids,
                   bool skip_steps,
                   bool only_final_result,
                   int target_cluster_id)
{
  auto macro_placer = getMacroPlacer();
  std::unique_ptr<MplObserver> graphics
    = std::make_unique<Graphics>(coarse, fine, block, ord::getLogger());
  macro_placer->setDebug(graphics);
  macro_placer->setDebugShowBundledNets(show_bundled_nets);
  macro_placer->setDebugShowClustersIds(show_clusters_ids);
  macro_placer->setDebugSkipSteps(skip_steps);
  macro_placer->setDebugOnlyFinalResult(only_final_result);
  macro_placer->setDebugTargetClusterId(target_cluster_id);
}

void
place_macro(odb::dbInst* inst,
            float x_origin,
            float y_origin,
            std::string orientation_string,
            bool exact,
            bool allow_overlap)
{
  odb::dbOrientType orientation(orientation_string.c_str());

  getMacroPlacer()->placeMacro(
    inst, x_origin, y_origin, orientation, exact, allow_overlap);
}

void
add_guidance_region(odb::dbInst* macro,
                    float x1,
                    float y1,
                    float x2,
                    float y2)
{
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  odb::Rect region = odb::Rect(block->micronsToDbu(x1),
                              block->micronsToDbu(y1),
                              block->micronsToDbu(x2),
                              block->micronsToDbu(y2));
  getMacroPlacer()->addGuidanceRegion(macro, region);
}

void
set_macro_halo(odb::dbInst* macro, 
               float width, 
               float height) 
{
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  int width_dbu = block->micronsToDbu(width);
  int height_dbu = block->micronsToDbu(height);

  getMacroPlacer()->setMacroHalo(macro, width_dbu, height_dbu);
}

void
set_macro_placement_file(std::string file_name)
{
  getMacroPlacer()->setMacroPlacementFile(file_name);
}

} // namespace

%} // inline
