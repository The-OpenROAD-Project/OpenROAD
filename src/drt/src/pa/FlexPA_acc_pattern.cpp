// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frTime.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frMPin.h"
#include "db/obj/frVia.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "omp.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "serialization.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

void FlexPA::buildInstsSet()
{
  insts_set_.clear();
  std::set<frInst*> target_frinsts;
  for (auto inst : target_insts_) {
    target_frinsts.insert(design_->getTopBlock()->findInst(inst));
  }
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    if (!target_insts_.empty()
        && target_frinsts.find(inst.get()) == target_frinsts.end()) {
      continue;
    }
    addToInstsSet(inst.get());
  }
}

void FlexPA::removeFromInstsSet(frInst* inst)
{
  // find then erase
  auto it = insts_set_.find(inst);
  bool found = it != insts_set_.end() && (*it) == inst;
  if (found) {
    insts_set_.erase(it);
  }
}

void FlexPA::addToInstsSet(frInst* inst)
{
  if (insts_set_.find(inst) != insts_set_.end()) {
    return;
  }
  if (!unique_insts_.hasUnique(inst)) {
    return;
  }
  if (!isStdCell(inst)) {
    return;
  }
  if (isSkipInst(inst)) {
    return;
  }
  insts_set_.insert(inst);
}

void FlexPA::prepPatternInst(frInst* unique_inst)
{
  int num_valid_pattern = prepPatternInstHelper(unique_inst, true);

  if (num_valid_pattern > 0) {
    return;
  }
  num_valid_pattern = prepPatternInstHelper(unique_inst, false);

  if (num_valid_pattern == 0) {
    logger_->warn(DRT,
                  87,
                  "No valid pattern for unique instance {}, master is {}.",
                  unique_inst->getName(),
                  unique_inst->getMaster()->getName());
  }
}

// the input inst must be unique instance
int FlexPA::prepPatternInstHelper(frInst* unique_inst, const bool use_x)
{
  std::vector<std::pair<frCoord, std::pair<frMPin*, frInstTerm*>>> pins;
  // TODO: add assert in case input inst is not unique inst
  int pin_access_idx = unique_inst->getPinAccessIdx();
  for (auto& inst_term : unique_inst->getInstTerms()) {
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
        const double coord = (use_x ? sum_x_coord : sum_y_coord) / (double) cnt;
        pins.push_back({(int) std::round(coord), {pin.get(), inst_term.get()}});
      }
    }
    if (n_aps == 0 && !inst_term->getTerm()->getPins().empty()) {
      logger_->error(DRT,
                     86,
                     "Term {} ({}) does not have any access point.",
                     inst_term->getName(),
                     unique_inst->getMaster()->getName());
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

  return genPatterns(unique_inst, pin_inst_term_pairs);
}

int FlexPA::genPatterns(
    frInst* unique_inst,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins)
{
  if (pins.empty()) {
    return -1;
  }

  int max_access_point_size = 0;
  int pin_access_idx = pins[0].second->getInst()->getPinAccessIdx();
  for (auto& [pin, inst_term] : pins) {
    max_access_point_size
        = std::max(max_access_point_size,
                   pin->getPinAccess(pin_access_idx)->getNumAccessPoints());
  }
  if (max_access_point_size == 0) {
    return 0;
  }

  // moved for mt
  std::set<std::vector<int>> inst_access_patterns;
  std::set<std::pair<int, int>> used_access_points;
  std::set<std::pair<int, int>> viol_access_points;
  int num_valid_pattern = 0;

  num_valid_pattern += FlexPA::genPatternsHelper(unique_inst,
                                                 pins,
                                                 inst_access_patterns,
                                                 used_access_points,
                                                 viol_access_points,
                                                 max_access_point_size);
  // try reverse order if no valid pattern
  if (num_valid_pattern == 0) {
    auto reversed_pins = pins;
    reverse(reversed_pins.begin(), reversed_pins.end());

    num_valid_pattern += FlexPA::genPatternsHelper(unique_inst,
                                                   reversed_pins,
                                                   inst_access_patterns,
                                                   used_access_points,
                                                   viol_access_points,
                                                   max_access_point_size);
  }

  return num_valid_pattern;
}

int FlexPA::genPatternsHelper(
    frInst* unique_inst,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int max_access_point_size)
{
  int num_node = (pins.size() + 2) * max_access_point_size;
  int num_edge = num_node * max_access_point_size;
  int num_valid_pattern = 0;

  std::vector<std::vector<std::unique_ptr<FlexDPNode>>> nodes(pins.size() + 2);
  std::vector<int> vio_edge(num_edge, -1);

  genPatternsInit(nodes,
                  pins,
                  inst_access_patterns,
                  used_access_points,
                  viol_access_points);

  for (int i = 0; i < router_cfg_->ACCESS_PATTERN_END_ITERATION_NUM; i++) {
    genPatternsReset(nodes, pins);
    genPatternsPerform(unique_inst,
                       nodes,
                       pins,
                       vio_edge,
                       used_access_points,
                       viol_access_points,
                       max_access_point_size);
    bool is_valid = false;
    if (genPatternsCommit(unique_inst,
                          nodes,
                          pins,
                          is_valid,
                          inst_access_patterns,
                          used_access_points,
                          viol_access_points,
                          max_access_point_size)) {
      if (is_valid) {
        num_valid_pattern++;
      } else {
      }
    } else {
      break;
    }
  }
  return num_valid_pattern;
}

// init dp node array for valid access points
void FlexPA::genPatternsInit(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points)
{
  // clear temp storage and flag
  inst_access_patterns.clear();
  used_access_points.clear();
  viol_access_points.clear();

  // init virtual nodes
  const int source_node_idx = pins.size() + 1;
  nodes[source_node_idx] = std::vector<std::unique_ptr<FlexDPNode>>(1);
  nodes[source_node_idx][0] = std::make_unique<FlexDPNode>();
  FlexDPNode* source_node = nodes[source_node_idx][0].get();
  source_node->setNodeCost(0);
  source_node->setIdx({pins.size() + 1, 0});
  source_node->setPathCost(0);
  source_node->setAsSource();

  const int sink_node_idx = pins.size();
  nodes[sink_node_idx] = std::vector<std::unique_ptr<FlexDPNode>>(1);
  nodes[sink_node_idx][0] = std::make_unique<FlexDPNode>();
  FlexDPNode* sink_node = nodes[sink_node_idx][0].get();
  sink_node->setNodeCost(0);
  sink_node->setIdx({pins.size(), 0});
  sink_node->setAsSink();
  // init pin nodes
  int pin_idx = 0;
  int ap_idx = 0;
  int pin_access_idx = pins[0].second->getInst()->getPinAccessIdx();

  for (auto& [pin, inst_term] : pins) {
    ap_idx = 0;
    auto size = pin->getPinAccess(pin_access_idx)->getAccessPoints().size();
    nodes[pin_idx] = std::vector<std::unique_ptr<FlexDPNode>>(size);
    for (auto& ap : pin->getPinAccess(pin_access_idx)->getAccessPoints()) {
      nodes[pin_idx][ap_idx] = std::make_unique<FlexDPNode>();
      nodes[pin_idx][ap_idx]->setIdx({pin_idx, ap_idx});
      nodes[pin_idx][ap_idx]->setNodeCost(ap->getCost());
      ap_idx++;
    }
    pin_idx++;
  }
}

void FlexPA::genPatternsReset(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins)
{
  for (auto& pin_nodes : nodes) {
    for (auto& node : pin_nodes) {
      node->setPathCost(std::numeric_limits<int>::max());
      node->setPrevNode(nullptr);
    }
  }

  FlexDPNode* source_node = nodes[pins.size() + 1][0].get();
  source_node->setNodeCost(0);
  source_node->setPathCost(0);

  FlexDPNode* sink_node = nodes[pins.size()][0].get();
  sink_node->setNodeCost(0);
}

bool FlexPA::genPatternsGC(
    const std::set<frBlockObject*>& target_objs,
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    const PatternType pattern_type,
    // NOLINTNEXTLINE(readability-non-const-parameter)
    std::set<frBlockObject*>* owners)
{
  if (objs.empty()) {
    if (router_cfg_->VERBOSE > 1) {
      logger_->warn(DRT, 89, "genPattern_gc objs empty.");
    }
    return true;
  }

  FlexGCWorker design_rule_checker(getTech(), logger_, router_cfg_);
  design_rule_checker.setIgnoreMinArea();
  design_rule_checker.setIgnoreLongSideEOL();
  design_rule_checker.setIgnoreCornerSpacing();

  frCoord llx = std::numeric_limits<frCoord>::max();
  frCoord lly = std::numeric_limits<frCoord>::max();
  frCoord urx = std::numeric_limits<frCoord>::min();
  frCoord ury = std::numeric_limits<frCoord>::min();
  for (auto& [connFig, owner] : objs) {
    odb::Rect bbox = connFig->getBBox();
    llx = std::min(llx, bbox.xMin());
    lly = std::min(lly, bbox.yMin());
    urx = std::max(urx, bbox.xMax());
    ury = std::max(ury, bbox.yMax());
  }
  const odb::Rect ext_box(llx - 3000, lly - 3000, urx + 3000, ury + 3000);
  design_rule_checker.setExtBox(ext_box);
  design_rule_checker.setDrcBox(ext_box);

  design_rule_checker.setTargetObjs(target_objs);
  if (target_objs.empty()) {
    design_rule_checker.setIgnoreDB();
  }
  design_rule_checker.initPA0(getDesign());
  for (auto& [connFig, owner] : objs) {
    design_rule_checker.addPAObj(connFig, owner);
  }
  design_rule_checker.initPA1();
  design_rule_checker.main();

  const bool no_drv = design_rule_checker.getMarkers().empty();
  if (owners) {
    for (auto& marker : design_rule_checker.getMarkers()) {
      for (auto& src : marker->getSrcs()) {
        owners->insert(src);
      }
    }
  }
  if (graphics_) {
    graphics_->setObjsAndMakers(
        objs, design_rule_checker.getMarkers(), pattern_type);
  }
  return no_drv;
}

void FlexPA::genPatternsPerform(
    frInst* unique_inst,
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int max_access_point_size)
{
  const int source_node_idx = pins.size() + 1;
  for (int curr_pin_idx = 0; curr_pin_idx <= (int) pins.size();
       curr_pin_idx++) {
    for (int curr_acc_point_idx = 0;
         curr_acc_point_idx < (int) nodes[curr_pin_idx].size();
         curr_acc_point_idx++) {
      FlexDPNode* curr_node = nodes[curr_pin_idx][curr_acc_point_idx].get();
      if (curr_node->getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      int prev_pin_idx = curr_pin_idx > 0 ? curr_pin_idx - 1 : source_node_idx;
      for (int prev_acc_point_idx = 0;
           prev_acc_point_idx < nodes[prev_pin_idx].size();
           prev_acc_point_idx++) {
        FlexDPNode* prev_node = nodes[prev_pin_idx][prev_acc_point_idx].get();
        if (prev_node->getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        const int edge_cost = getEdgeCost(unique_inst,
                                          prev_node,
                                          curr_node,
                                          pins,
                                          vio_edges,
                                          used_access_points,
                                          viol_access_points,
                                          max_access_point_size);
        if (curr_node->getPathCost() == std::numeric_limits<int>::max()
            || curr_node->getPathCost()
                   > prev_node->getPathCost() + edge_cost) {
          curr_node->setPathCost(prev_node->getPathCost() + edge_cost);
          curr_node->setPrevNode(prev_node);
        }
      }
    }
  }
}

int FlexPA::getEdgeCost(
    frInst* unique_inst,
    FlexDPNode* prev_node,
    FlexDPNode* curr_node,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int max_access_point_size)
{
  int edge_cost = 0;
  auto [prev_pin_idx, prev_acc_point_idx] = prev_node->getIdx();
  auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();

  if (prev_node->isSource() || curr_node->isSink()) {
    return edge_cost;
  }

  bool has_vio = false;
  // check if the edge has been calculated
  int edge_idx = getFlatEdgeIdx(prev_pin_idx,
                                prev_acc_point_idx,
                                curr_acc_point_idx,
                                max_access_point_size);
  if (vio_edges[edge_idx] != -1) {
    has_vio = (vio_edges[edge_idx] == 1);
  } else {
    odb::dbTransform xform = unique_inst->getNoRotationTransform();
    // check DRC
    std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
    const auto& [pin_1, inst_term_1] = pins[prev_pin_idx];
    const auto target_obj = inst_term_1->getInst();
    const int pin_access_idx = target_obj->getPinAccessIdx();
    const auto pa_1 = pin_1->getPinAccess(pin_access_idx);
    const frAccessPoint* ap_1 = pa_1->getAccessPoint(prev_acc_point_idx);
    std::unique_ptr<frVia> via1;
    if (ap_1->hasAccess(frDirEnum::U)) {
      odb::Point pt1(ap_1->getPoint());
      xform.apply(pt1);
      via1 = std::make_unique<frVia>(ap_1->getViaDef(), pt1);
      via1->setOrigin(pt1);
      if (inst_term_1->hasNet()) {
        objs.emplace_back(via1.get(), inst_term_1->getNet());
      } else {
        objs.emplace_back(via1.get(), inst_term_1);
      }
    }

    const auto& [pin_2, inst_term_2] = pins[curr_pin_idx];
    const auto pa_2 = pin_2->getPinAccess(pin_access_idx);
    const frAccessPoint* ap_2 = pa_2->getAccessPoint(curr_acc_point_idx);
    std::unique_ptr<frVia> via2;
    if (ap_2->hasAccess(frDirEnum::U)) {
      odb::Point pt2(ap_2->getPoint());
      xform.apply(pt2);
      via2 = std::make_unique<frVia>(ap_2->getViaDef(), pt2);
      if (inst_term_2->hasNet()) {
        objs.emplace_back(via2.get(), inst_term_2->getNet());
      } else {
        objs.emplace_back(via2.get(), inst_term_2);
      }
    }

    has_vio = !genPatternsGC({target_obj}, objs, Edge);
    vio_edges[edge_idx] = has_vio;

    // look back for GN14
    if (!has_vio) {
      // check one more back
      if (prev_node->hasPrevNode()) {
        auto prev_prev_node = prev_node->getPrevNode();
        auto [prev_prev_pin_idx, prev_prev_acc_point_idx]
            = prev_prev_node->getIdx();
        if (!prev_prev_node->isSource()) {
          const auto& [pin_3, inst_term_3] = pins[prev_prev_pin_idx];
          const auto pa_3 = pin_3->getPinAccess(pin_access_idx);
          const frAccessPoint* ap_3
              = pa_3->getAccessPoint(prev_prev_acc_point_idx);
          std::unique_ptr<frVia> via3;
          if (ap_3->hasAccess(frDirEnum::U)) {
            odb::Point pt3(ap_3->getPoint());
            xform.apply(pt3);
            via3 = std::make_unique<frVia>(ap_3->getViaDef(), pt3);
            if (inst_term_3->hasNet()) {
              objs.emplace_back(via3.get(), inst_term_3->getNet());
            } else {
              objs.emplace_back(via3.get(), inst_term_3);
            }
          }

          has_vio = !genPatternsGC({target_obj}, objs, Edge);
        }
      }
    }
  }

  if (!has_vio) {
    if ((prev_pin_idx == 0
         && used_access_points.find(
                std::make_pair(prev_pin_idx, prev_acc_point_idx))
                != used_access_points.end())
        || (curr_pin_idx == (int) pins.size() - 1
            && used_access_points.find(
                   std::make_pair(curr_pin_idx, curr_acc_point_idx))
                   != used_access_points.end())) {
      edge_cost = 100;
    } else if (viol_access_points.find(
                   std::make_pair(prev_pin_idx, prev_acc_point_idx))
                   != viol_access_points.end()
               || viol_access_points.find(
                      std::make_pair(curr_pin_idx, curr_acc_point_idx))
                      != viol_access_points.end()) {
      edge_cost = 1000;
    } else {
      const int prev_node_cost = prev_node->getNodeCost();
      const int curr_node_cost = curr_node->getNodeCost();
      edge_cost = (prev_node_cost + curr_node_cost) / 2;
    }
  } else {
    edge_cost = 1000 /*violation cost*/;
  }

  return edge_cost;
}

std::vector<int> FlexPA::extractAccessPatternFromNodes(
    frInst* inst,
    const std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::pair<int, int>>& used_access_points)
{
  std::vector<int> access_pattern(pins.size(), -1);

  const FlexDPNode* source_node = nodes[pins.size() + 1][0].get();
  const FlexDPNode* sink_node = nodes[pins.size()][0].get();

  FlexDPNode* curr_node = sink_node->getPrevNode();

  while (curr_node != source_node) {
    if (!curr_node) {
      logger_->error(DRT,
                     90,
                     "Valid access pattern not found for inst {}({}).",
                     inst->getName(),
                     inst->getMaster()->getName());
    }

    auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
    access_pattern[curr_pin_idx] = curr_acc_point_idx;
    used_access_points.insert({curr_pin_idx, curr_acc_point_idx});

    curr_node = curr_node->getPrevNode();
  }
  return access_pattern;
}

bool FlexPA::genPatternsCommit(
    frInst* unique_inst,
    const std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    bool& is_valid,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int max_access_point_size)
{
  std::vector<int> access_pattern = extractAccessPatternFromNodes(
      unique_inst, nodes, pins, used_access_points);
  // not a new access pattern
  if (inst_access_patterns.find(access_pattern) != inst_access_patterns.end()) {
    return false;
  }

  inst_access_patterns.insert(access_pattern);
  // create new access pattern and push to uniqueInstances
  auto pin_access_pattern = std::make_unique<FlexPinAccessPattern>();
  std::map<frMPin*, frAccessPoint*> pin_to_access_point;
  // check DRC for the whole pattern
  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
  std::vector<std::unique_ptr<frVia>> temp_vias;
  frInst* target_obj = nullptr;
  for (int pin_idx = 0; pin_idx < (int) pins.size(); pin_idx++) {
    auto acc_point_idx = access_pattern[pin_idx];
    auto& [pin, inst_term] = pins[pin_idx];
    target_obj = unique_inst;
    const int pin_access_idx = unique_inst->getPinAccessIdx();
    const auto pa = pin->getPinAccess(pin_access_idx);
    const auto access_point = pa->getAccessPoint(acc_point_idx);
    pin_to_access_point[pin] = access_point;

    // add objs
    std::unique_ptr<frVia> via;
    if (access_point->hasAccess(frDirEnum::U)) {
      via = std::make_unique<frVia>(access_point->getViaDef());
      auto rvia = via.get();
      temp_vias.push_back(std::move(via));

      odb::dbTransform xform = unique_inst->getNoRotationTransform();
      odb::Point pt(access_point->getPoint());
      xform.apply(pt);
      rvia->setOrigin(pt);
      if (inst_term->hasNet()) {
        objs.emplace_back(rvia, inst_term->getNet());
      } else {
        objs.emplace_back(rvia, inst_term);
      }
    }
  }

  frAccessPoint* left_access_point = nullptr;
  frAccessPoint* right_access_point = nullptr;
  frCoord left_pt = std::numeric_limits<frCoord>::max();
  frCoord right_pt = std::numeric_limits<frCoord>::min();

  for (auto& inst_term : unique_inst->getInstTerms()) {
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }
    uint64_t n_no_ap_pins = 0;
    for (auto& pin : inst_term->getTerm()->getPins()) {
      if (pin_to_access_point.find(pin.get()) == pin_to_access_point.end()) {
        n_no_ap_pins++;
        pin_access_pattern->addAccessPoint(nullptr);
      } else {
        const auto& ap = pin_to_access_point[pin.get()];
        const odb::Point tmpPt = ap->getPoint();
        if (tmpPt.x() < left_pt) {
          left_access_point = ap;
          left_pt = tmpPt.x();
        }
        if (tmpPt.x() > right_pt) {
          right_access_point = ap;
          right_pt = tmpPt.x();
        }
        pin_access_pattern->addAccessPoint(ap);
      }
    }
    if (n_no_ap_pins == inst_term->getTerm()->getPins().size()) {
      logger_->error(DRT,
                     91,
                     "{} does not have valid access points.",
                     inst_term->getName());
    }
  }
  pin_access_pattern->setBoundaryAP(true, left_access_point);
  pin_access_pattern->setBoundaryAP(false, right_access_point);

  std::set<frBlockObject*> owners;
  if (target_obj != nullptr
      && genPatternsGC({target_obj}, objs, Commit, &owners)) {
    pin_access_pattern->updateCost();
    unique_inst_patterns_.at(unique_insts_.getUniqueClass(unique_inst))
        .push_back(std::move(pin_access_pattern));
    // genPatternsPrint(nodes, pins);
    is_valid = true;
  } else {
    for (int pin_idx = 0; pin_idx < (int) pins.size(); pin_idx++) {
      auto acc_pattern_idx = access_pattern[pin_idx];
      auto inst_term = pins[pin_idx].second;
      frBlockObject* owner = inst_term;
      if (inst_term->hasNet()) {
        owner = inst_term->getNet();
      }
      if (owners.find(owner) != owners.end()) {
        viol_access_points.insert({pin_idx, acc_pattern_idx});  // idx ;
      }
    }
  }

  // new access pattern
  return true;
}

void FlexPA::genPatternsPrintDebug(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins)
{
  FlexDPNode* sink_node = nodes[pins.size() + 1][0].get();
  FlexDPNode* curr_node = sink_node;
  int pin_cnt = pins.size();

  odb::dbTransform xform;
  auto& [pin, inst_term] = pins[0];
  if (inst_term) {
    frInst* unique_inst = inst_term->getInst();
    xform = unique_inst->getNoRotationTransform();
  }

  std::cout << "failed pattern:";

  double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  while (curr_node->hasPrevNode()) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      auto& [pin, inst_term] = pins[pin_cnt];
      auto unique_inst = inst_term->getInst();
      std::cout << " " << inst_term->getTerm()->getName();
      const int pin_access_idx = unique_inst->getPinAccessIdx();
      auto pa = pin->getPinAccess(pin_access_idx);
      auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
      odb::Point pt(pa->getAccessPoint(curr_acc_point_idx)->getPoint());
      xform.apply(pt);
      std::cout << " (" << pt.x() / dbu << ", " << pt.y() / dbu << ")";
    }

    curr_node = curr_node->getPrevNode();
    pin_cnt--;
  }
  std::cout << std::endl;
  if (pin_cnt != -1) {
    logger_->error(DRT, 277, "Valid access pattern not found.");
  }
}

void FlexPA::genPatternsPrint(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins)
{
  FlexDPNode* sink_node = nodes[pins.size() + 1][0].get();
  FlexDPNode* curr_node = sink_node;
  int pin_cnt = pins.size();

  std::cout << "new pattern\n";

  while (curr_node->hasPrevNode()) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      auto& [pin, inst_term] = pins[pin_cnt];
      auto unique_inst = inst_term->getInst();
      const int pin_access_idx = unique_inst->getPinAccessIdx();
      auto pa = pin->getPinAccess(pin_access_idx);
      auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
      std::unique_ptr<frVia> via = std::make_unique<frVia>(
          pa->getAccessPoint(curr_acc_point_idx)->getViaDef());
      odb::Point pt(pa->getAccessPoint(curr_acc_point_idx)->getPoint());
      std::cout << " gccleanvia " << unique_inst->getMaster()->getName() << " "
                << inst_term->getTerm()->getName() << " "
                << via->getViaDef()->getName() << " " << pt.x() << " " << pt.y()
                << " " << unique_inst->getOrient().getString() << "\n";
    }

    curr_node = curr_node->getPrevNode();
    pin_cnt--;
  }
  if (pin_cnt != -1) {
    logger_->error(DRT, 278, "Valid access pattern not found.");
  }
}

// get flat edge index
int FlexPA::getFlatEdgeIdx(const int prev_idx_1,
                           const int prev_idx_2,
                           const int curr_idx_2,
                           const int idx_2_dim)
{
  return ((prev_idx_1 + 1) * idx_2_dim + prev_idx_2) * idx_2_dim + curr_idx_2;
}

}  // namespace drt
