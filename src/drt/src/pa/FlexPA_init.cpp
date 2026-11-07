// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/infra/frTime.h"
#include "db/obj/frInst.h"
#include "frBaseTypes.h"
#include "gc/FlexGC.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "pa/FlexPA.h"
#include "pa/FlexPA_unique.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {
namespace {

constexpr int kMsgIdPinAccessCoordinateDomain = 6014;

}  // namespace

void FlexPA::initViaRawPriority()
{
  layer_num_to_via_defs_.clear();
  for (int layer_num = design_->getTech()->getBottomLayerNum();
       layer_num <= design_->getTech()->getTopLayerNum();
       ++layer_num) {
    if (design_->getTech()->getLayer(layer_num)->getType()
        != dbTechLayerType::CUT) {
      continue;
    }
    for (const auto& via_def :
         design_->getTech()->getLayer(layer_num)->getViaDefs()) {
      const int cutNum = int(via_def->getCutFigs().size());
      const ViaRawPriorityTuple priority = getViaRawPriority(via_def);
      layer_num_to_via_defs_[layer_num][cutNum][priority].push_back(via_def);
    }
    for (auto& [cut_num, priorities] : layer_num_to_via_defs_[layer_num]) {
      for (auto& [priority, vias] : priorities) {
        // Priority describes geometry and preference, not via identity.
        std::ranges::stable_sort(vias, [](const auto* lhs, const auto* rhs) {
          return lhs->getName() < rhs->getName();
        });
      }
    }
  }
}

ViaRawPriorityTuple FlexPA::getViaRawPriority(const frViaDef* via_def) const
{
  const bool is_not_default_via = !(via_def->getDefault());
  gtl::polygon_90_set_data<frCoord> via_layer_ps1;

  for (auto& fig : via_def->getLayer1Figs()) {
    const odb::Rect bbox = fig->getBBox();
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
    const odb::Rect bbox = fig->getBBox();
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
  const int64_t manu_grid = getDesign()->getTech()->getManufacturingGrid();

  // full coords
  track_coords_.clear();
  track_coords_.resize(num_layers);
  for (auto& track_pattern : design_->getTopBlock()->getTrackPatterns()) {
    const auto layer_num = track_pattern->getLayerNum();
    const bool is_vertical_layer
        = design_->getTech()->getLayer(layer_num)->getDir()
          == dbTechLayerDir::VERTICAL;
    const bool is_vertical_track
        = track_pattern->isHorizontal();  // true = vertical track
    if (is_vertical_layer != is_vertical_track) {
      continue;
    }
    const int64_t count = track_pattern->getNumTracks();
    const int64_t start = track_pattern->getStartCoord();
    const int64_t spacing = track_pattern->getTrackSpacing();
    if (count > std::numeric_limits<int>::max()) {
      logger_->error(DRT,
                     kMsgIdPinAccessCoordinateDomain,
                     "Pin-access track count on layer {} exceeds INT_MAX.",
                     layer_num);
    }
    if (count != 0
        && start + (count - 1) * spacing
               > std::numeric_limits<frCoord>::max()) {
      logger_->error(DRT,
                     kMsgIdPinAccessCoordinateDomain,
                     "Last pin-access track coordinate on layer {} is outside "
                     "the native coordinate range.",
                     layer_num);
    }
    for (int64_t i = 0; i < count; ++i) {
      const auto coordinate = static_cast<frCoord>(start + i * spacing);
      track_coords_[layer_num][coordinate] = frAccessPointEnum::OnGrid;
    }
  }

  // half coords
  if (manu_grid <= 0 || manu_grid > std::numeric_limits<frCoord>::max()) {
    logger_->error(DRT,
                   kMsgIdPinAccessCoordinateDomain,
                   "Manufacturing grid {} is invalid for pin-access track "
                   "midpoint generation.",
                   manu_grid);
  }
  std::vector<std::vector<frCoord>> half_track_coords(num_layers);
  for (int i = 0; i < num_layers; i++) {
    frCoord prev_full_coord = std::numeric_limits<frCoord>::max();

    for (auto& [curr_full_coord, cost] : track_coords_[i]) {
      if (curr_full_coord > prev_full_coord) {
        const int64_t midpoint
            = (static_cast<int64_t>(prev_full_coord) + curr_full_coord) / 2;
        const auto curr_half_grid
            = static_cast<frCoord>(midpoint / manu_grid * manu_grid);
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

void FlexPA::initAllSkipInstTerm()
{
  const auto& unique = unique_insts_.getUniqueClasses();
#pragma omp parallel for schedule(dynamic)
  for (const auto& unique_class : unique) {
    initSkipInstTerm(unique_class.get());
  }
}

void FlexPA::initSkipInstTerm(UniqueClass* unique_class)
{
  for (const auto& term : unique_class->getMaster()->getTerms()) {
    bool skip = true;
    for (const auto& inst : unique_class->getInsts()) {
      auto inst_term = inst->getInstTerm(term->getIndexInOwner());
      skip = isSkipInstTermLocal(inst_term);
      if (!skip) {
        break;
      }
    }
    unique_class->setSkipTerm(term.get(), skip);
  }
}

bool FlexPA::updateSkipInstTerm(frInst* inst)
{
  auto unique_class = unique_insts_.getUniqueClass(inst);
  for (const auto& term : inst->getInstTerms()) {
    if (!isSkipInstTermLocal(term.get())
        && unique_class->isSkipTerm(term->getTerm())) {
      return true;
    }
  }
  return false;
}

}  // namespace drt
