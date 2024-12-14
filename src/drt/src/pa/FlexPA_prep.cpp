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

#include <omp.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "FlexPA.h"
#include "FlexPA_graphics.h"
#include "db/infra/frTime.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "serialization.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

// TODO there should be a better way to get this info by getting the master
// terms from OpenDB
bool FlexPA::isStdCell(frInst* inst)
{
  return inst->getMaster()->getMasterType().isCore();
}

bool FlexPA::isMacroCell(frInst* inst)
{
  dbMasterType masterType = inst->getMaster()->getMasterType();
  return (masterType.isBlock() || masterType.isPad()
          || masterType == dbMasterType::RING);
}

template <typename T>
std::vector<gtl::polygon_90_set_data<frCoord>>
FlexPA::mergePinShapes(T* pin, frInstTerm* inst_term, const bool is_shrink)
{
  frInst* inst = nullptr;
  if (inst_term) {
    inst = inst_term->getInst();
  }

  dbTransform xform;
  if (inst) {
    xform = inst->getDBTransform();
  }

  frTechObject* tech = getDesign()->getTech();
  std::size_t num_layers = tech->getLayers().size();

  std::vector<frCoord> layer_widths;
  if (is_shrink) {
    layer_widths.resize(num_layers, 0);
    for (int i = 0; i < int(layer_widths.size()); i++) {
      layer_widths[i] = tech->getLayer(i)->getWidth();
    }
  }

  std::vector<gtl::polygon_90_set_data<frCoord>> pin_shapes(num_layers);

  for (auto& shape : pin->getFigs()) {
    if (shape->typeId() == frcRect) {
      auto obj = static_cast<frRect*>(shape.get());
      auto layer_num = obj->getLayerNum();
      auto layer = tech->getLayer(layer_num);
      dbTechLayerDir dir = layer->getDir();
      if (layer->getType() != dbTechLayerType::ROUTING) {
        continue;
      }
      Rect box = obj->getBBox();
      xform.apply(box);
      gtl::rectangle_data<frCoord> rect(
          box.xMin(), box.yMin(), box.xMax(), box.yMax());
      if (is_shrink) {
        if (dir == dbTechLayerDir::HORIZONTAL) {
          gtl::shrink(rect, gtl::VERTICAL, layer_widths[layer_num] / 2);
        } else if (dir == dbTechLayerDir::VERTICAL) {
          gtl::shrink(rect, gtl::HORIZONTAL, layer_widths[layer_num] / 2);
        }
      }
      using boost::polygon::operators::operator+=;
      pin_shapes[layer_num] += rect;
    } else if (shape->typeId() == frcPolygon) {
      auto obj = static_cast<frPolygon*>(shape.get());
      auto layer_num = obj->getLayerNum();
      std::vector<gtl::point_data<frCoord>> points;
      // must be copied pts
      for (Point pt : obj->getPoints()) {
        xform.apply(pt);
        points.emplace_back(pt.x(), pt.y());
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      using boost::polygon::operators::operator+=;
      pin_shapes[layer_num] += poly;
    } else {
      logger_->error(DRT, 67, "FlexPA mergePinShapes unsupported shape.");
    }
  }
  return pin_shapes;
}

bool FlexPA::enclosesOnTrackPlanarAccess(
    const gtl::rectangle_data<frCoord>& rect,
    frLayerNum layer_num)
{
  frCoord low, high;
  frLayer* layer = getDesign()->getTech()->getLayer(layer_num);
  if (layer->isHorizontal()) {
    low = gtl::yl(rect);
    high = gtl::yh(rect);
  } else if (layer->isVertical()) {
    low = gtl::xl(rect);
    high = gtl::xh(rect);
  } else {
    logger_->error(
        DRT,
        1003,
        "enclosesPlanarAccess: layer is neither vertical or horizontal");
  }
  const auto& tracks = track_coords_[layer_num];
  const auto low_track = tracks.lower_bound(low);
  if (low_track == tracks.end()) {
    logger_->error(DRT, 1004, "enclosesPlanarAccess: low track not found");
  }
  if (low_track->first > high) {
    return false;
  }
  auto high_track = tracks.lower_bound(high);
  if (high_track != tracks.end()) {
    if (high_track->first > high) {
      high_track--;
    }
  } else {
    logger_->error(DRT, 1005, "enclosesPlanarAccess: high track not found");
  }
  if (high_track->first - low_track->first > (int) layer->getPitch()) {
    return true;
  }
  if (low_track->first - (int) layer->getWidth() / 2 < low) {
    return false;
  }
  if (high_track->first + (int) layer->getWidth() / 2 > high) {
    return false;
  }
  return true;
}

template <typename T>
void FlexPA::check_setAPsAccesses(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const bool& is_std_cell_pin)
{
  std::vector<std::vector<gtl::polygon_90_data<frCoord>>> layer_polys(
      pin_shapes.size());
  for (int i = 0; i < (int) pin_shapes.size(); i++) {
    pin_shapes[i].get_polygons(layer_polys[i]);
  }
  bool has_access = false;
  for (auto& ap : aps) {
    const auto layer_num = ap->getLayerNum();
    check_addAccess(ap.get(),
                    pin_shapes[layer_num],
                    layer_polys[layer_num],
                    pin,
                    inst_term);
    if (is_std_cell_pin) {
      has_access |= ((layer_num == router_cfg_->VIA_ACCESS_LAYERNUM
                      && ap->hasAccess(frDirEnum::U))
                     || (layer_num != router_cfg_->VIA_ACCESS_LAYERNUM
                         && ap->hasAccess()));
    } else {
      has_access |= ap->hasAccess();
    }
  }
  if (!has_access) {
    for (auto& ap : aps) {
      const auto layer_num = ap->getLayerNum();
      check_addAccess(ap.get(),
                      pin_shapes[layer_num],
                      layer_polys[layer_num],
                      pin,
                      inst_term,
                      true);
    }
  }
}

template <typename T>
void FlexPA::updatePinStats(
    const std::vector<std::unique_ptr<frAccessPoint>>& tmp_aps,
    T* pin,
    frInstTerm* inst_term)
{
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    is_std_cell_pin = isStdCell(inst_term->getInst());
    is_macro_cell_pin = isMacroCell(inst_term->getInst());
  }
  for (auto& ap : tmp_aps) {
    if (ap->hasAccess(frDirEnum::W) || ap->hasAccess(frDirEnum::E)
        || ap->hasAccess(frDirEnum::S) || ap->hasAccess(frDirEnum::N)) {
      if (is_std_cell_pin) {
#pragma omp atomic
        std_cell_pin_valid_planar_ap_cnt_++;
      }
      if (is_macro_cell_pin) {
#pragma omp atomic
        macro_cell_pin_valid_planar_ap_cnt_++;
      }
    }
    if (ap->hasAccess(frDirEnum::U)) {
      if (is_std_cell_pin) {
#pragma omp atomic
        std_cell_pin_valid_via_ap_cnt_++;
      }
      if (is_macro_cell_pin) {
#pragma omp atomic
        macro_cell_pin_valid_via_ap_cnt_++;
      }
    }
  }
}

template <typename T>
bool FlexPA::initPinAccessCostBounded(
    std::vector<std::unique_ptr<frAccessPoint>>& aps,
    std::set<std::pair<Point, frLayerNum>>& apset,
    std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
    T* pin,
    frInstTerm* inst_term,
    const frAccessPointEnum lower_type,
    const frAccessPointEnum upper_type)
{
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    is_std_cell_pin = isStdCell(inst_term->getInst());
    is_macro_cell_pin = isMacroCell(inst_term->getInst());
  }
  const bool is_io_pin = (inst_term == nullptr);
  std::vector<std::unique_ptr<frAccessPoint>> tmp_aps;
  genAPsFromPinShapes(
      tmp_aps, apset, pin, inst_term, pin_shapes, lower_type, upper_type);
  check_setAPsAccesses(tmp_aps, pin_shapes, pin, inst_term, is_std_cell_pin);
  if (is_std_cell_pin) {
#pragma omp atomic
    std_cell_pin_gen_ap_cnt_ += tmp_aps.size();
  }
  if (is_macro_cell_pin) {
#pragma omp atomic
    macro_cell_pin_gen_ap_cnt_ += tmp_aps.size();
  }
  if (graphics_) {
    graphics_->setAPs(tmp_aps, lower_type, upper_type);
  }
  for (auto& ap : tmp_aps) {
    // for stdcell, add (i) planar access if layer_num != VIA_ACCESS_LAYERNUM,
    // and (ii) access if exist access for macro, allow pure planar ap
    if (is_std_cell_pin) {
      const auto layer_num = ap->getLayerNum();
      if ((layer_num == router_cfg_->VIA_ACCESS_LAYERNUM
           && ap->hasAccess(frDirEnum::U))
          || (layer_num != router_cfg_->VIA_ACCESS_LAYERNUM
              && ap->hasAccess())) {
        aps.push_back(std::move(ap));
      }
    } else if ((is_macro_cell_pin || is_io_pin) && ap->hasAccess()) {
      aps.push_back(std::move(ap));
    }
  }
  int n_sparse_access_points = (int) aps.size();
  Rect tbx;
  for (int i = 0; i < (int) aps.size();
       i++) {  // not perfect but will do the job
    int r = design_->getTech()->getLayer(aps[i]->getLayerNum())->getWidth() / 2;
    tbx.init(
        aps[i]->x() - r, aps[i]->y() - r, aps[i]->x() + r, aps[i]->y() + r);
    for (int j = i + 1; j < (int) aps.size(); j++) {
      if (aps[i]->getLayerNum() == aps[j]->getLayerNum()
          && tbx.intersects(aps[j]->getPoint())) {
        n_sparse_access_points--;
        break;
      }
    }
  }
  if (is_std_cell_pin
      && n_sparse_access_points >= router_cfg_->MINNUMACCESSPOINT_STDCELLPIN) {
    updatePinStats(aps, pin, inst_term);
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  if (is_macro_cell_pin
      && n_sparse_access_points
             >= router_cfg_->MINNUMACCESSPOINT_MACROCELLPIN) {
    updatePinStats(aps, pin, inst_term);
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  if (is_io_pin && (int) aps.size() > 0) {
    // IO term pin always only have one access
    for (auto& ap : aps) {
      pin->getPinAccess(0)->addAccessPoint(std::move(ap));
    }
    return true;
  }
  return false;
}

// first create all access points with costs
template <typename T>
int FlexPA::initPinAccess(T* pin, frInstTerm* inst_term)
{
  // aps are after xform
  // before checkPoints, ap->hasAccess(dir) indicates whether to check drc
  std::vector<std::unique_ptr<frAccessPoint>> aps;
  std::set<std::pair<Point, frLayerNum>> apset;
  bool is_std_cell_pin = false;
  bool is_macro_cell_pin = false;
  if (inst_term) {
    is_std_cell_pin = isStdCell(inst_term->getInst());
    is_macro_cell_pin = isMacroCell(inst_term->getInst());
  }

  if (graphics_) {
    std::set<frInst*, frBlockObjectComp>* inst_class = nullptr;
    if (inst_term) {
      inst_class = unique_insts_.getClass(inst_term->getInst());
    }
    graphics_->startPin(pin, inst_term, inst_class);
  }

  std::vector<gtl::polygon_90_set_data<frCoord>> pin_shapes
      = mergePinShapes(pin, inst_term);

  for (auto upper : {frAccessPointEnum::OnGrid,
                     frAccessPointEnum::HalfGrid,
                     frAccessPointEnum::Center,
                     frAccessPointEnum::EncOpt,
                     frAccessPointEnum::NearbyGrid}) {
    for (auto lower : {frAccessPointEnum::OnGrid,
                       frAccessPointEnum::HalfGrid,
                       frAccessPointEnum::Center,
                       frAccessPointEnum::EncOpt}) {
      if (upper == frAccessPointEnum::NearbyGrid && !aps.empty()) {
        // Only use NearbyGrid as a last resort (at least until
        // nangate45/aes is resolved).
        continue;
      }
      if (initPinAccessCostBounded(
              aps, apset, pin_shapes, pin, inst_term, lower, upper)) {
        return aps.size();
      }
    }
  }

  // inst_term aps are written back here if not early stopped
  // IO term aps are are written back in initPinAccessCostBounded and always
  // early stopped
  updatePinStats(aps, pin, inst_term);
  const int n_aps = aps.size();
  if (n_aps == 0) {
    if (is_std_cell_pin) {
      std_cell_pin_no_ap_cnt_++;
    }
    if (is_macro_cell_pin) {
      macro_cell_pin_no_ap_cnt_++;
    }
  } else {
    if (inst_term == nullptr) {
      logger_->error(DRT, 254, "inst_term can not be nullptr");
    }
    // write to pa
    const int pin_access_idx = unique_insts_.getPAIndex(inst_term->getInst());
    for (auto& ap : aps) {
      pin->getPinAccess(pin_access_idx)->addAccessPoint(std::move(ap));
    }
  }
  return n_aps;
}

static inline void serializePatterns(
    const std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern>>>&
        patterns,
    const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << patterns;
  file.close();
}

void FlexPA::initInstAccessPoints(frInst* inst)
{
  ProfileTask profile("PA:uniqueInstance");
  for (auto& inst_term : inst->getInstTerms()) {
    // only do for normal and clock terms
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }
    int n_aps = 0;
    for (auto& pin : inst_term->getTerm()->getPins()) {
      n_aps += initPinAccess(pin.get(), inst_term.get());
    }
    if (!n_aps) {
      logger_->error(DRT,
                     73,
                     "No access point for {}/{}.",
                     inst_term->getInst()->getName(),
                     inst_term->getTerm()->getName());
    }
  }
}

void FlexPA::initAllAccessPoints()
{
  ProfileTask profile("PA:point");
  int pin_count = 0;

  omp_set_num_threads(router_cfg_->MAX_THREADS);
  ThreadException exception;

  const std::vector<frInst*>& unique = unique_insts_.getUnique();
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int) unique.size(); i++) {  // NOLINT
    try {
      frInst* inst = unique[i];

      // only do for core and block cells
      if (!isStdCell(inst) && !isMacroCell(inst)) {
        continue;
      }

      initInstAccessPoints(inst);
      if (router_cfg_->VERBOSE <= 0) {
        continue;
      }

      int inst_terms_cnt = static_cast<int>(inst->getInstTerms().size());
#pragma omp critical
      for (int i = 0; i < inst_terms_cnt; i++) {
        pin_count++;
        if (pin_count % (pin_count > 10000 ? 10000 : 1000) == 0) {
          logger_->info(DRT, 76, "  Complete {} pins.", pin_count);
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();

  // PA for IO terms
  if (target_insts_.empty()) {
    omp_set_num_threads(router_cfg_->MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
    for (unsigned i = 0;  // NOLINT
         i < getDesign()->getTopBlock()->getTerms().size();
         i++) {
      try {
        auto& term = getDesign()->getTopBlock()->getTerms()[i];
        if (term->getType().isSupply()) {
          continue;
        }
        auto net = term->getNet();
        if (!net || net->isSpecial()) {
          continue;
        }
        int n_aps = 0;
        for (auto& pin : term->getPins()) {
          n_aps += initPinAccess(pin.get(), nullptr);
        }
        if (!n_aps) {
          logger_->error(
              DRT, 74, "No access point for PIN/{}.", term->getName());
        }
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
  }

  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 78, "  Complete {} pins.", pin_count);
  }
}

void FlexPA::prepPattern()
{
  ProfileTask profile("PA:pattern");

  const auto& unique = unique_insts_.getUnique();

  // revert access points to origin
  unique_inst_patterns_.resize(unique.size());

  int cnt = 0;

  omp_set_num_threads(router_cfg_->MAX_THREADS);
  ThreadException exception;
#pragma omp parallel for schedule(dynamic)
  for (int curr_unique_inst_idx = 0; curr_unique_inst_idx < (int) unique.size();
       curr_unique_inst_idx++) {
    try {
      auto& inst = unique[curr_unique_inst_idx];
      // only do for core and block cells
      // TODO the above comment says "block cells" but that's not what the code
      // does?
      if (!isStdCell(inst)) {
        continue;
      }

      int num_valid_pattern = prepPatternInst(inst, curr_unique_inst_idx, 1.0);

      if (num_valid_pattern == 0) {
        // In FAx1_ASAP7_75t_R (in asap7) the pins are mostly horizontal
        // and sorting in X works poorly.  So we try again sorting in Y.
        num_valid_pattern = prepPatternInst(inst, curr_unique_inst_idx, 0.0);
        if (num_valid_pattern == 0) {
          logger_->warn(
              DRT,
              87,
              "No valid pattern for unique instance {}, master is {}.",
              inst->getName(),
              inst->getMaster()->getName());
        }
      }
#pragma omp critical
      {
        cnt++;
        if (router_cfg_->VERBOSE > 0) {
          if (cnt % (cnt > 1000 ? 1000 : 100) == 0) {
            logger_->info(DRT, 79, "  Complete {} unique inst patterns.", cnt);
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 81, "  Complete {} unique inst patterns.", cnt);
  }
  if (isDistributed()) {
    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
        result;
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    std::string patterns_file = fmt::format("{}/patterns.bin", shared_vol_);
    serializePatterns(unique_inst_patterns_, patterns_file);
    uDesc->setPath(patterns_file);
    uDesc->setType(PinAccessJobDescription::UPDATE_PATTERNS);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(
          utl::DRT, 330, "Error sending UPDATE_PATTERNS Job to cloud");
    }
  }

  // prep pattern for each row
  std::vector<frInst*> insts;
  std::vector<std::vector<frInst*>> inst_rows;
  std::vector<frInst*> row_insts;

  auto instLocComp = [](frInst* const& a, frInst* const& b) {
    const Point originA = a->getOrigin();
    const Point originB = b->getOrigin();
    if (originA.y() == originB.y()) {
      return (originA.x() < originB.x());
    }
    return (originA.y() < originB.y());
  };

  getInsts(insts);
  std::sort(insts.begin(), insts.end(), instLocComp);

  // gen rows of insts
  int prev_y_coord = INT_MIN;
  int prev_x_end_coord = INT_MIN;
  for (auto inst : insts) {
    Point origin = inst->getOrigin();
    if (origin.y() != prev_y_coord || origin.x() > prev_x_end_coord) {
      if (!row_insts.empty()) {
        inst_rows.push_back(row_insts);
        row_insts.clear();
      }
    }
    row_insts.push_back(inst);
    prev_y_coord = origin.y();
    Rect inst_boundary_box = inst->getBoundaryBBox();
    prev_x_end_coord = inst_boundary_box.xMax();
  }
  if (!row_insts.empty()) {
    inst_rows.push_back(row_insts);
  }
  prepPatternInstRows(std::move(inst_rows));
}

void FlexPA::revertAccessPoints()
{
  const auto& unique = unique_insts_.getUnique();
  for (auto& inst : unique) {
    const dbTransform xform = inst->getTransform();
    const Point offset(xform.getOffset());
    dbTransform revertXform(Point(-offset.getX(), -offset.getY()));

    const auto pin_access_idx = unique_insts_.getPAIndex(inst);
    for (auto& inst_term : inst->getInstTerms()) {
      // if (isSkipInstTerm(inst_term.get())) {
      //   continue;
      // }

      for (auto& pin : inst_term->getTerm()->getPins()) {
        auto pin_access = pin->getPinAccess(pin_access_idx);
        for (auto& access_point : pin_access->getAccessPoints()) {
          Point unique_AP_point(access_point->getPoint());
          revertXform.apply(unique_AP_point);
          access_point->setPoint(unique_AP_point);
          for (auto& ps : access_point->getPathSegs()) {
            Point begin = ps.getBeginPoint();
            Point end = ps.getEndPoint();
            revertXform.apply(begin);
            revertXform.apply(end);
            if (end < begin) {
              Point tmp = begin;
              begin = end;
              end = tmp;
            }
            ps.setPoints(begin, end);
          }
        }
      }
    }
  }
}

void FlexPA::getInsts(std::vector<frInst*>& insts)
{
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst->getName()));
  }
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    if (!unique_insts_.hasUnique(inst.get())) {
      continue;
    }
    if (!isStdCell(inst.get())) {
      continue;
    }
    bool is_skip = true;
    for (auto& inst_term : inst->getInstTerms()) {
      if (!isSkipInstTerm(inst_term.get())) {
        is_skip = false;
        break;
      }
    }
    if (!is_skip) {
      insts.push_back(inst.get());
    }
  }
}

// Skip power pins, pins connected to special nets, and dangling pins
// (since we won't route these).
//
// Checks only this inst_term and not an equivalent ones.  This
// is a helper to isSkipInstTerm and initSkipInstTerm.
bool FlexPA::isSkipInstTermLocal(frInstTerm* in)
{
  auto term = in->getTerm();
  if (term->getType().isSupply()) {
    return true;
  }
  auto in_net = in->getNet();
  if (in_net && !in_net->isSpecial()) {
    return false;
  }
  return true;
}

bool FlexPA::isSkipInstTerm(frInstTerm* in)
{
  auto inst_class = unique_insts_.getClass(in->getInst());
  if (inst_class == nullptr) {
    return isSkipInstTermLocal(in);
  }

  // This should be already computed in initSkipInstTerm()
  return skip_unique_inst_term_.at({inst_class, in->getTerm()});
}

// the input inst must be unique instance
int FlexPA::prepPatternInst(frInst* inst,
                            const int curr_unique_inst_idx,
                            const double x_weight)
{
  std::vector<std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>> pins;
  // TODO: add assert in case input inst is not unique inst
  int pin_access_idx = unique_insts_.getPAIndex(inst);
  for (auto& inst_term : inst->getInstTerms()) {
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }
    int n_aps = 0;
    for (auto& pin : inst_term->getTerm()->getPins()) {
      // container of access points
      auto pin_access = pin->getPinAccess(pin_access_idx);
      int sum_x_coord = 0;
      int sum_y_coord = 0;
      int cnt = 0;
      // get avg x coord for sort
      for (auto& access_point : pin_access->getAccessPoints()) {
        sum_x_coord += access_point->getPoint().x();
        sum_y_coord += access_point->getPoint().y();
        cnt++;
      }
      n_aps += cnt;
      if (cnt != 0) {
        const double coord
            = (x_weight * sum_x_coord + (1.0 - x_weight) * sum_y_coord) / cnt;
        pins.push_back({(int) std::round(coord), {pin.get(), inst_term.get()}});
      }
    }
    if (n_aps == 0 && !inst_term->getTerm()->getPins().empty()) {
      logger_->error(DRT, 86, "Pin does not have an access point.");
    }
  }
  std::sort(pins.begin(),
            pins.end(),
            [](const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& lhs,
               const std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>& rhs) {
              return lhs.first < rhs.first;
            });

  std::vector<std::pair<frMPin*, frInstTerm*>> pin_inst_term_pairs;
  pin_inst_term_pairs.reserve(pins.size());
  for (auto& [x, m] : pins) {
    pin_inst_term_pairs.push_back(m);
  }

  return genPatterns(pin_inst_term_pairs, curr_unique_inst_idx);
}

}  // namespace drt
