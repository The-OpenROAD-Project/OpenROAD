// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "mpl/rtl_mp.h"
#include "MplObserver.h"
#include "graphics.h"
#include "odb/db.h"
#include "utl/options.h"

namespace ord {
// Defined in OpenRoad.i
mpl::MacroPlacer*
getMacroPlacer();
utl::Logger* getLogger();
}

static mpl::MacroPlacerOptions getOptions(
    odb::dbBlock* block,
    const std::map<std::string, std::string>& keys,
    const std::map<std::string, std::string>& flags)
{
  using utl::checkFlag;
  using utl::checkKey;

  mpl::MacroPlacerOptions options;
  checkKey(keys, "-max_num_macro", options.max_num_macro);
  checkKey(keys, "-min_num_macro", options.min_num_macro);
  checkKey(keys, "-max_num_inst", options.max_num_inst);
  checkKey(keys, "-min_num_inst", options.min_num_inst);
  checkKey(keys, "-tolerance", options.tolerance);
  checkKey(keys, "-max_num_level", options.max_num_level);
  checkKey(keys, "-coarsening_ratio", options.coarsening_ratio);
  checkKey(keys, "-large_net_threshold", options.large_net_threshold);

  float fence_lx = 0.0f, fence_ly = 0.0f, fence_ux = 0.0f, fence_uy = 0.0f;
  checkKey(keys, "-fence_lx", fence_lx);
  checkKey(keys, "-fence_ly", fence_ly);
  checkKey(keys, "-fence_ux", fence_ux);
  checkKey(keys, "-fence_uy", fence_uy);
  options.global_fence = odb::Rect(block->micronsToDbu(fence_lx),
                                  block->micronsToDbu(fence_ly),
                                  block->micronsToDbu(fence_ux),
                                  block->micronsToDbu(fence_uy));

  checkKey(keys, "-area_weight", options.area_weight);
  checkKey(keys, "-outline_weight", options.outline_weight);
  checkKey(keys, "-wirelength_weight", options.wirelength_weight);
  checkKey(keys, "-guidance_weight", options.guidance_weight);
  checkKey(keys, "-fence_weight", options.fence_weight);
  checkKey(keys, "-boundary_weight", options.boundary_weight);
  checkKey(keys, "-notch_weight", options.notch_weight);
  checkKey(keys, "-soft_blockage_weight", options.soft_blockage_weight);
  checkKey(keys, "-target_util", options.target_util);
  checkKey(keys, "-min_ar", options.min_ar);
  checkKey(keys, "-report_directory", options.report_directory);
  checkKey(keys, "-write_macro_placement", options.macro_placement_file);

  checkFlag(flags, "-keep_clustering_data", options.keep_clustering_data);
  checkFlag(flags, "-pin_aware_channels", options.pin_aware_channels);

  return options;
}

static mpl::MacroPlacerDebugOptions getDebugOptions(
    odb::dbBlock* block,
    const std::map<std::string, std::string>& keys,
    const std::map<std::string, std::string>& flags)
{
  using utl::checkFlag;
  using utl::checkKey;

  // Default to both coarse and fine when neither is given.
  bool coarse = false;
  bool fine = false;
  checkFlag(flags, "-coarse", coarse);
  checkFlag(flags, "-fine", fine);
  if (!coarse && !fine) {
    coarse = true;
    fine = true;
  }

  mpl::MacroPlacerDebugOptions options;
  options.graphics
      = std::make_unique<mpl::Graphics>(coarse, fine, block, ord::getLogger());
  checkFlag(flags, "-show_bundled_nets", options.show_bundled_nets);
  checkFlag(flags, "-show_clusters_ids", options.show_clusters_ids);
  checkFlag(flags, "-skip_steps", options.skip_steps);
  checkFlag(flags, "-only_final_result", options.only_final_result);
  checkKey(keys, "-target_cluster_id", options.target_cluster_id);

  return options;
}

using utl::MPL;
using ord::getMacroPlacer;
%}

%include "../../Exception.i"
%include "../../options.i"

%inline %{

namespace mpl {

bool rtl_macro_placer_cmd(const std::map<std::string, std::string>& keys,
                          const std::map<std::string, std::string>& flags) {
  auto macro_placer = getMacroPlacer();
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  const int threads = ord::OpenRoad::openRoad()->getThreadCount();

  mpl::MacroPlacerOptions options = getOptions(block, keys, flags);

  return macro_placer->place(threads, options);
}

void set_debug_cmd(const std::map<std::string, std::string>& keys,
                   const std::map<std::string, std::string>& flags)
{
  auto macro_placer = getMacroPlacer();
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();

  macro_placer->configureDebug(getDebugOptions(block, keys, flags));
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
set_min_channel(float width,
                float height)
{
  odb::dbBlock* block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  getMacroPlacer()->setMinChannelSize(block->micronsToDbu(width), block->micronsToDbu(height));
}

void
block_macro_channels()
{
  getMacroPlacer()->blockMacroChannels();
}

} // namespace

%} // inline
