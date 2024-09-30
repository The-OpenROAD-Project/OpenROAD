/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "db/infra/frTime.h"
#include "gc/FlexGC.h"

namespace drt {

void FlexPA::initViaRawPriority()
{
  for (int layer_num = design_->getTech()->getBottomLayerNum();
       layer_num <= design_->getTech()->getTopLayerNum();
       ++layer_num) {
    if (design_->getTech()->getLayer(layer_num)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    for (auto& via_def :
         design_->getTech()->getLayer(layer_num)->getViaDefs()) {
      const int cutNum = int(via_def->getCutFigs().size());
      const ViaRawPriorityTuple priority = getViaRawPriority(via_def);
      layer_num_to_via_defs_[layer_num][cutNum][priority] = via_def;
    }
  }
}

ViaRawPriorityTuple FlexPA::getViaRawPriority(frViaDef* via_def)
{
  const bool is_not_default_via = !(via_def->getDefault());
  gtl::polygon_90_set_data<frCoord> via_layer_ps1;

  for (auto& fig : via_def->getLayer1Figs()) {
    const Rect bbox = fig->getBBox();
    gtl::rectangle_data<frCoord> bbox_rect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using boost::polygon::operators::operator+=;
    via_layer_ps1 += bbox_rect;
  }
  gtl::rectangle_data<frCoord> layer1_rect;
  gtl::extents(layer1_rect, via_layer_ps1);
  const bool is_layer1_horz = (gtl::xh(layer1_rect) - gtl::xl(layer1_rect))
                              > (gtl::yh(layer1_rect) - gtl::yl(layer1_rect));
  const frCoord layer1_width
      = std::min((gtl::xh(layer1_rect) - gtl::xl(layer1_rect)),
                 (gtl::yh(layer1_rect) - gtl::yl(layer1_rect)));

  const auto layer1_num = via_def->getLayer1Num();
  const auto dir1 = getDesign()->getTech()->getLayer(layer1_num)->getDir();

  const bool is_not_lower_align
      = (is_layer1_horz && (dir1 == dbTechLayerDir::VERTICAL))
        || (!is_layer1_horz && (dir1 == dbTechLayerDir::HORIZONTAL));

  gtl::polygon_90_set_data<frCoord> via_layer_PS2;
  for (auto& fig : via_def->getLayer2Figs()) {
    const Rect bbox = fig->getBBox();
    const gtl::rectangle_data<frCoord> bbox_rect(
        bbox.xMin(), bbox.yMin(), bbox.xMax(), bbox.yMax());
    using boost::polygon::operators::operator+=;
    via_layer_PS2 += bbox_rect;
  }
  gtl::rectangle_data<frCoord> layer2_rect;
  gtl::extents(layer2_rect, via_layer_PS2);
  const bool is_layer2_horz = (gtl::xh(layer2_rect) - gtl::xl(layer2_rect))
                              > (gtl::yh(layer2_rect) - gtl::yl(layer2_rect));
  const frCoord layer2_width
      = std::min((gtl::xh(layer2_rect) - gtl::xl(layer2_rect)),
                 (gtl::yh(layer2_rect) - gtl::yl(layer2_rect)));

  const auto layer2_num = via_def->getLayer2Num();
  const auto dir2 = getDesign()->getTech()->getLayer(layer2_num)->getDir();

  const bool is_not_upper_align
      = (is_layer2_horz && (dir2 == dbTechLayerDir::VERTICAL))
        || (!is_layer2_horz && (dir2 == dbTechLayerDir::HORIZONTAL));

  const frCoord layer1_area = gtl::area(via_layer_ps1);
  const frCoord layer2_area = gtl::area(via_layer_PS2);

  return std::make_tuple(is_not_default_via,
                         layer1_width,
                         layer2_width,
                         is_not_upper_align,
                         layer2_area,
                         layer1_area,
                         is_not_lower_align);
}

void FlexPA::initTrackCoords()
{
  const int num_layers = getDesign()->getTech()->getLayers().size();
  const frCoord manu_grid = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  track_coords_.clear();
  track_coords_.resize(num_layers);
  for (auto& track_pattern : design_->getTopBlock()->getTrackPatterns()) {
    const auto layer_num = track_pattern->getLayerNum();
    const auto is_vert_layer
        = (design_->getTech()->getLayer(layer_num)->getDir()
           == dbTechLayerDir::VERTICAL);
    const auto is_vert_track
        = track_pattern->isHorizontal();  // true = vertical track
    if ((!is_vert_layer && !is_vert_track)
        || (is_vert_layer && is_vert_track)) {
      frCoord curr_coord = track_pattern->getStartCoord();
      for (int i = 0; i < (int) track_pattern->getNumTracks(); i++) {
        track_coords_[layer_num][curr_coord] = frAccessPointEnum::OnGrid;
        curr_coord += track_pattern->getTrackSpacing();
      }
    }
  }

  // half coords
  std::vector<std::vector<frCoord>> half_track_coords(num_layers);
  for (int i = 0; i < num_layers; i++) {
    frCoord prev_full_coord = std::numeric_limits<frCoord>::max();

    for (auto& [curr_full_coord, cost] : track_coords_[i]) {
      if (curr_full_coord > prev_full_coord) {
        const frCoord curr_half_grid
            = (curr_full_coord + prev_full_coord) / 2 / manu_grid * manu_grid;
        if (curr_half_grid != curr_full_coord
            && curr_half_grid != prev_full_coord) {
          half_track_coords[i].push_back(curr_half_grid);
        }
      }
      prev_full_coord = curr_full_coord;
    }
    for (auto half_coord : half_track_coords[i]) {
      track_coords_[i][half_coord] = frAccessPointEnum::HalfGrid;
    }
  }
}

void FlexPA::initSkipInstTerm()
{
  const auto& unique = unique_insts_.getUnique();

  // Populate the map single-threaded so no further resizing is needed.
  for (frInst* inst : unique) {
    for (auto& inst_term : inst->getInstTerms()) {
      auto term = inst_term->getTerm();
      auto inst_class = unique_insts_.getClass(inst);
      skip_unique_inst_term_[{inst_class, term}] = false;
    }
  }

  const int unique_size = unique.size();
#pragma omp parallel for schedule(dynamic)
  for (int unique_inst_idx = 0; unique_inst_idx < unique_size;
       unique_inst_idx++) {
    frInst* inst = unique[unique_inst_idx];
    for (auto& inst_term : inst->getInstTerms()) {
      frMTerm* term = inst_term->getTerm();
      const UniqueInsts::InstSet* inst_class = unique_insts_.getClass(inst);

      // We have to be careful that the skip conditions are true not only of
      // the unique instance but also all the equivalent instances.
      bool skip = isSkipInstTermLocal(inst_term.get());
      if (skip) {
        for (frInst* inst : *inst_class) {
          frInstTerm* it = inst->getInstTerm(inst_term->getIndexInOwner());
          skip = isSkipInstTermLocal(it);
          if (!skip) {
            break;
          }
        }
      }
      skip_unique_inst_term_.at({inst_class, term}) = skip;
    }
  }
}

}  // namespace drt
