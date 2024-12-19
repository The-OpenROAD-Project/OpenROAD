/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * Copyright (c) 2024, Precision Innovations Inc.
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

int FlexPA::genPatterns(
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    int curr_unique_inst_idx)
{
  if (pins.empty()) {
    return -1;
  }

  int max_access_point_size = 0;
  int pin_access_idx = unique_insts_.getPAIndex(pins[0].second->getInst());
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

  num_valid_pattern += FlexPA::genPatterns_helper(pins,
                                                  inst_access_patterns,
                                                  used_access_points,
                                                  viol_access_points,
                                                  curr_unique_inst_idx,
                                                  max_access_point_size);
  // try reverse order if no valid pattern
  if (num_valid_pattern == 0) {
    auto reversed_pins = pins;
    reverse(reversed_pins.begin(), reversed_pins.end());

    num_valid_pattern += FlexPA::genPatterns_helper(reversed_pins,
                                                    inst_access_patterns,
                                                    used_access_points,
                                                    viol_access_points,
                                                    curr_unique_inst_idx,
                                                    max_access_point_size);
  }

  return num_valid_pattern;
}

int FlexPA::genPatterns_helper(
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
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
    genPatterns_reset(nodes, pins);
    genPatterns_perform(nodes,
                        pins,
                        vio_edge,
                        used_access_points,
                        viol_access_points,
                        curr_unique_inst_idx,
                        max_access_point_size);
    bool is_valid = false;
    if (genPatterns_commit(nodes,
                           pins,
                           is_valid,
                           inst_access_patterns,
                           used_access_points,
                           viol_access_points,
                           curr_unique_inst_idx,
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
  int pin_access_idx = unique_insts_.getPAIndex(pins[0].second->getInst());

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

void FlexPA::genPatterns_reset(
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

bool FlexPA::genPatterns_gc(
    const std::set<frBlockObject*>& target_objs,
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    const PatternType pattern_type,
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
    Rect bbox = connFig->getBBox();
    llx = std::min(llx, bbox.xMin());
    lly = std::min(lly, bbox.yMin());
    urx = std::max(urx, bbox.xMax());
    ury = std::max(ury, bbox.yMax());
  }
  const Rect ext_box(llx - 3000, lly - 3000, urx + 3000, ury + 3000);
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
  design_rule_checker.end();

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

void FlexPA::genPatterns_perform(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
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

        const int edge_cost = getEdgeCost(prev_node,
                                          curr_node,
                                          pins,
                                          vio_edges,
                                          used_access_points,
                                          viol_access_points,
                                          curr_unique_inst_idx,
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
    FlexDPNode* prev_node,
    FlexDPNode* curr_node,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    std::vector<int>& vio_edges,
    const std::set<std::pair<int, int>>& used_access_points,
    const std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
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
    auto curr_unique_inst = unique_insts_.getUnique(curr_unique_inst_idx);
    dbTransform xform = curr_unique_inst->getNoRotationTransform();
    // check DRC
    std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
    const auto& [pin_1, inst_term_1] = pins[prev_pin_idx];
    const auto target_obj = inst_term_1->getInst();
    const int pin_access_idx = unique_insts_.getPAIndex(target_obj);
    const auto pa_1 = pin_1->getPinAccess(pin_access_idx);
    std::unique_ptr<frVia> via1;
    if (pa_1->getAccessPoint(prev_acc_point_idx)->hasAccess(frDirEnum::U)) {
      via1 = std::make_unique<frVia>(
          pa_1->getAccessPoint(prev_acc_point_idx)->getViaDef());
      Point pt1(pa_1->getAccessPoint(prev_acc_point_idx)->getPoint());
      xform.apply(pt1);
      via1->setOrigin(pt1);
      if (inst_term_1->hasNet()) {
        objs.emplace_back(via1.get(), inst_term_1->getNet());
      } else {
        objs.emplace_back(via1.get(), inst_term_1);
      }
    }

    const auto& [pin_2, inst_term_2] = pins[curr_pin_idx];
    const auto pa_2 = pin_2->getPinAccess(pin_access_idx);
    std::unique_ptr<frVia> via2;
    if (pa_2->getAccessPoint(curr_acc_point_idx)->hasAccess(frDirEnum::U)) {
      via2 = std::make_unique<frVia>(
          pa_2->getAccessPoint(curr_acc_point_idx)->getViaDef());
      Point pt2(pa_2->getAccessPoint(curr_acc_point_idx)->getPoint());
      xform.apply(pt2);
      via2->setOrigin(pt2);
      if (inst_term_2->hasNet()) {
        objs.emplace_back(via2.get(), inst_term_2->getNet());
      } else {
        objs.emplace_back(via2.get(), inst_term_2);
      }
    }

    has_vio = !genPatterns_gc({target_obj}, objs, Edge);
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
          auto pa_3 = pin_3->getPinAccess(pin_access_idx);
          std::unique_ptr<frVia> via3;
          if (pa_3->getAccessPoint(prev_prev_acc_point_idx)
                  ->hasAccess(frDirEnum::U)) {
            via3 = std::make_unique<frVia>(
                pa_3->getAccessPoint(prev_prev_acc_point_idx)->getViaDef());
            Point pt3(
                pa_3->getAccessPoint(prev_prev_acc_point_idx)->getPoint());
            xform.apply(pt3);
            via3->setOrigin(pt3);
            if (inst_term_3->hasNet()) {
              objs.emplace_back(via3.get(), inst_term_3->getNet());
            } else {
              objs.emplace_back(via3.get(), inst_term_3);
            }
          }

          has_vio = !genPatterns_gc({target_obj}, objs, Edge);
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
      logger_->error(DRT, 90, "Valid access pattern not found.");
    }

    auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
    access_pattern[curr_pin_idx] = curr_acc_point_idx;
    used_access_points.insert({curr_pin_idx, curr_acc_point_idx});

    curr_node = curr_node->getPrevNode();
  }
  return access_pattern;
}

bool FlexPA::genPatterns_commit(
    const std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
    bool& is_valid,
    std::set<std::vector<int>>& inst_access_patterns,
    std::set<std::pair<int, int>>& used_access_points,
    std::set<std::pair<int, int>>& viol_access_points,
    const int curr_unique_inst_idx,
    const int max_access_point_size)
{
  std::vector<int> access_pattern
      = extractAccessPatternFromNodes(nodes, pins, used_access_points);
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
    auto inst = inst_term->getInst();
    target_obj = inst;
    const int pin_access_idx = unique_insts_.getPAIndex(inst);
    const auto pa = pin->getPinAccess(pin_access_idx);
    const auto access_point = pa->getAccessPoint(acc_point_idx);
    pin_to_access_point[pin] = access_point;

    // add objs
    std::unique_ptr<frVia> via;
    if (access_point->hasAccess(frDirEnum::U)) {
      via = std::make_unique<frVia>(access_point->getViaDef());
      auto rvia = via.get();
      temp_vias.push_back(std::move(via));

      dbTransform xform = inst->getNoRotationTransform();
      Point pt(access_point->getPoint());
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

  const auto& [pin, inst_term] = pins[0];
  const auto inst = inst_term->getInst();
  for (auto& inst_term : inst->getInstTerms()) {
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
        const Point tmpPt = ap->getPoint();
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
      && genPatterns_gc({target_obj}, objs, Commit, &owners)) {
    pin_access_pattern->updateCost();
    unique_inst_patterns_[curr_unique_inst_idx].push_back(
        std::move(pin_access_pattern));
    // genPatterns_print(nodes, pins);
    is_valid = true;
  } else {
    for (int idx_1 = 0; idx_1 < (int) pins.size(); idx_1++) {
      auto idx_2 = access_pattern[idx_1];
      auto& [pin, inst_term] = pins[idx_1];
      if (inst_term->hasNet()) {
        if (owners.find(inst_term->getNet()) != owners.end()) {
          viol_access_points.insert(std::make_pair(idx_1, idx_2));  // idx ;
        }
      } else {
        if (owners.find(inst_term) != owners.end()) {
          viol_access_points.insert(std::make_pair(idx_1, idx_2));  // idx ;
        }
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

  dbTransform xform;
  auto& [pin, inst_term] = pins[0];
  if (inst_term) {
    frInst* inst = inst_term->getInst();
    xform = inst->getNoRotationTransform();
  }

  std::cout << "failed pattern:";

  double dbu = getDesign()->getTopBlock()->getDBUPerUU();
  while (curr_node->hasPrevNode()) {
    // non-virtual node
    if (pin_cnt != (int) pins.size()) {
      auto& [pin, inst_term] = pins[pin_cnt];
      auto inst = inst_term->getInst();
      std::cout << " " << inst_term->getTerm()->getName();
      const int pin_access_idx = unique_insts_.getPAIndex(inst);
      auto pa = pin->getPinAccess(pin_access_idx);
      auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
      Point pt(pa->getAccessPoint(curr_acc_point_idx)->getPoint());
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

void FlexPA::genPatterns_print(
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
      auto inst = inst_term->getInst();
      const int pin_access_idx = unique_insts_.getPAIndex(inst);
      auto pa = pin->getPinAccess(pin_access_idx);
      auto [curr_pin_idx, curr_acc_point_idx] = curr_node->getIdx();
      std::unique_ptr<frVia> via = std::make_unique<frVia>(
          pa->getAccessPoint(curr_acc_point_idx)->getViaDef());
      Point pt(pa->getAccessPoint(curr_acc_point_idx)->getPoint());
      std::cout << " gccleanvia " << inst->getMaster()->getName() << " "
                << inst_term->getTerm()->getName() << " "
                << via->getViaDef()->getName() << " " << pt.x() << " " << pt.y()
                << " " << inst->getOrient().getString() << "\n";
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