// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frFig.h"
#include "db/obj/frInst.h"
#include "db/obj/frVia.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "distributed/paUpdate.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "omp.h"
#include "pa/FlexPA.h"
#include "serialization.h"
#include "utl/Logger.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

static inline void serializeInstRows(
    const std::vector<std::vector<frInst*>>& inst_rows,
    const std::string& file_name)
{
  paUpdate update;
  update.setInstRows(inst_rows);
  paUpdate::serialize(update, file_name);
}

std::vector<std::vector<frInst*>> FlexPA::computeInstRows()
{
  // prep pattern for each row
  std::vector<std::vector<frInst*>> inst_rows;
  std::vector<frInst*> row_insts;

  buildInstsSet();

  // gen rows of insts
  int prev_y_coord = INT_MIN;
  int prev_x_end_coord = INT_MIN;
  for (auto inst : insts_set_) {
    odb::Point origin = inst->getBoundaryBBox().ll();
    if (origin.y() != prev_y_coord || origin.x() > prev_x_end_coord) {
      if (!row_insts.empty()) {
        inst_rows.push_back(row_insts);
        row_insts.clear();
      }
    }
    row_insts.push_back(inst);
    prev_y_coord = origin.y();
    odb::Rect inst_boundary_box = inst->getBoundaryBBox();
    prev_x_end_coord = inst_boundary_box.xMax();
  }
  if (!row_insts.empty()) {
    inst_rows.push_back(row_insts);
  }
  return inst_rows;
}

bool FlexPA::instancesAreAbuting(frInst* inst_1, frInst* inst_2) const
{
  if (inst_1->getOrigin().getY() != inst_2->getOrigin().getY()) {
    return false;
  }
  frInst *left_inst, *right_inst;
  if (inst_1->getOrigin().getX() < inst_2->getOrigin().getX()) {
    left_inst = inst_1;
    right_inst = inst_2;
  } else {
    left_inst = inst_2;
    right_inst = inst_1;
  }

  if (left_inst->getBoundaryBBox().xMax()
      != right_inst->getBoundaryBBox().xMin()) {
    return false;
  }

  return true;
}

std::vector<frInst*> FlexPA::getAdjacentInstancesCluster(frInst* inst) const
{
  const auto inst_it = insts_set_.find(inst);
  if (inst_it == insts_set_.end()) {
    logger_->error(
        DRT, 9419, "Inst {} not found in insts_set_", inst->getName());
  }
  std::vector<frInst*> adj_inst_cluster;

  adj_inst_cluster.push_back(inst);

  if (inst_it != insts_set_.begin()) {
    auto current_inst_it = inst_it;
    auto prev_inst_it = std::prev(inst_it);
    while (prev_inst_it != insts_set_.begin()
           && instancesAreAbuting(*current_inst_it, *prev_inst_it)) {
      adj_inst_cluster.push_back(*prev_inst_it);
      current_inst_it--;
      prev_inst_it--;
    }
  }

  std::reverse(adj_inst_cluster.begin(), adj_inst_cluster.end());
  if (inst_it != insts_set_.end()) {
    auto current_inst_it = inst_it;
    auto next_inst_it = std::next(inst_it);
    while (next_inst_it != insts_set_.end()
           && instancesAreAbuting(*current_inst_it, *next_inst_it)) {
      adj_inst_cluster.push_back(*next_inst_it);
      current_inst_it++;
      next_inst_it++;
    }
  }

  return adj_inst_cluster;
}

void FlexPA::prepPatternInstRows(std::vector<std::vector<frInst*>> inst_rows)
{
  ThreadException exception;
  int cnt = 0;
  if (isDistributed()) {
    omp_set_num_threads(cloud_sz_);
    const int batch_size = inst_rows.size() / cloud_sz_;
    paUpdate all_updates;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < cloud_sz_; i++) {
      try {
        std::vector<std::vector<frInst*>>::const_iterator start
            = inst_rows.begin() + (i * batch_size);
        std::vector<std::vector<frInst*>>::const_iterator end
            = (i == cloud_sz_ - 1) ? inst_rows.end() : start + batch_size;
        std::vector<std::vector<frInst*>> batch(start, end);
        std::string path = fmt::format("{}/batch_{}.bin", shared_vol_, i);
        serializeInstRows(batch, path);
        dst::JobMessage msg(dst::JobMessage::kPinAccess,
                            dst::JobMessage::kUnicast),
            result;
        std::unique_ptr<PinAccessJobDescription> uDesc
            = std::make_unique<PinAccessJobDescription>();
        uDesc->setPath(path);
        uDesc->setType(PinAccessJobDescription::INST_ROWS);
        msg.setJobDescription(std::move(uDesc));
        const bool ok
            = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
        if (!ok) {
          logger_->error(utl::DRT, 329, "Error sending INST_ROWS Job to cloud");
        }
        auto desc
            = static_cast<PinAccessJobDescription*>(result.getJobDescription());
        paUpdate update;
        paUpdate::deserialize(design_, update, desc->getPath());
        for (const auto& [term, aps] : update.getGroupResults()) {
          term->setAccessPoints(aps);
        }
#pragma omp critical
        {
          for (const auto& res : update.getGroupResults()) {
            all_updates.addGroupResult(res);
          }
          for (i = 0; i < batch.size(); i++) {
            cnt++;
            if (router_cfg_->VERBOSE > 0) {
              if (cnt % (cnt > 100000 ? 100000 : 10000) == 0) {
                logger_->info(DRT, 110, "  Complete {} groups.", cnt);
              }
            }
          }
        }
      } catch (...) {
        exception.capture();
      }
    }
    // send updates back to workers
    dst::JobMessage msg(dst::JobMessage::kPinAccess,
                        dst::JobMessage::kBroadcast),
        result;
    const std::string updates_path
        = fmt::format("{}/final_updates.bin", shared_vol_);
    paUpdate::serialize(all_updates, updates_path);
    std::unique_ptr<PinAccessJobDescription> uDesc
        = std::make_unique<PinAccessJobDescription>();
    uDesc->setPath(updates_path);
    uDesc->setType(PinAccessJobDescription::UPDATE_PA);
    msg.setJobDescription(std::move(uDesc));
    const bool ok
        = dist_->sendJob(msg, remote_host_.c_str(), remote_port_, result);
    if (!ok) {
      logger_->error(utl::DRT, 332, "Error sending UPDATE_PA Job to cloud");
    }
  } else {
    omp_set_num_threads(router_cfg_->MAX_THREADS);
    // choose access pattern of a row of insts
#pragma omp parallel for schedule(dynamic)
    for (auto& inst_row : inst_rows) {  // NOLINT
      try {
        genInstRowPattern(inst_row);
#pragma omp critical
        {
          cnt++;
          if (router_cfg_->VERBOSE > 0) {
            if (cnt % (cnt > 100000 ? 100000 : 10000) == 0) {
              logger_->info(DRT, 82, "  Complete {} groups.", cnt);
            }
          }
        }
      } catch (...) {
        exception.capture();
      }
    }
  }
  exception.rethrow();
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 84, "  Complete {} groups.", cnt);
  }
}

// calculate which pattern to be used for each inst
// the insts must be in the same row and sorted from left to right
void FlexPA::genInstRowPattern(std::vector<frInst*>& insts)
{
  if (insts.empty()) {
    return;
  }

  std::vector<std::vector<std::unique_ptr<FlexDPNode>>> nodes(insts.size() + 2);

  genInstRowPatternInit(nodes, insts);
  genInstRowPatternPerform(nodes, insts);
  genInstRowPatternCommit(nodes, insts);
  for (auto& inst : insts) {
    inst->setLatestPATransform();
    inst->setHasPinAccessUpdate(true);
  }
}

// init dp node array for valid access patterns
void FlexPA::genInstRowPatternInit(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<frInst*>& insts)
{
  // init virtual nodes
  const int source_node_idx = insts.size() + 1;
  nodes[source_node_idx] = std::vector<std::unique_ptr<FlexDPNode>>(1);
  nodes[source_node_idx][0] = std::make_unique<FlexDPNode>();
  nodes[source_node_idx][0]->setNodeCost(0);
  nodes[source_node_idx][0]->setPathCost(0);
  nodes[source_node_idx][0]->setIdx({insts.size() + 1, 0});
  nodes[source_node_idx][0]->setAsSource();

  const int sink_node_idx = insts.size();
  nodes[sink_node_idx] = std::vector<std::unique_ptr<FlexDPNode>>(1);
  nodes[sink_node_idx][0] = std::make_unique<FlexDPNode>();
  nodes[sink_node_idx][0]->setNodeCost(0);
  nodes[sink_node_idx][0]->setIdx({insts.size(), 0});
  nodes[sink_node_idx][0]->setAsSink();

  // init inst nodes
  for (int inst_idx = 0; inst_idx < (int) insts.size(); inst_idx++) {
    auto& inst = insts[inst_idx];
    auto unique_class = unique_insts_.getUniqueClass(inst);
    auto& inst_patterns = unique_inst_patterns_.at(unique_class);
    nodes[inst_idx]
        = std::vector<std::unique_ptr<FlexDPNode>>(inst_patterns.size());
    for (int acc_pattern_idx = 0; acc_pattern_idx < (int) inst_patterns.size();
         acc_pattern_idx++) {
      nodes[inst_idx][acc_pattern_idx] = std::make_unique<FlexDPNode>();
      auto access_pattern = inst_patterns[acc_pattern_idx].get();
      nodes[inst_idx][acc_pattern_idx]->setNodeCost(access_pattern->getCost());
      nodes[inst_idx][acc_pattern_idx]->setIdx({inst_idx, acc_pattern_idx});
    }
  }
}

void FlexPA::genInstRowPatternPerform(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<frInst*>& insts)
{
  const int source_node_idx = insts.size() + 1;
  for (int curr_inst_idx = 0; curr_inst_idx <= (int) insts.size();
       curr_inst_idx++) {
    for (int curr_acc_pattern_idx = 0;
         curr_acc_pattern_idx < nodes[curr_inst_idx].size();
         curr_acc_pattern_idx++) {
      FlexDPNode* curr_node = nodes[curr_inst_idx][curr_acc_pattern_idx].get();
      if (curr_node->getNodeCost() == std::numeric_limits<int>::max()) {
        continue;
      }
      const int prev_inst_idx
          = curr_inst_idx > 0 ? curr_inst_idx - 1 : source_node_idx;
      for (int prev_acc_pattern_idx = 0;
           prev_acc_pattern_idx < nodes[prev_inst_idx].size();
           prev_acc_pattern_idx++) {
        FlexDPNode* prev_node
            = nodes[prev_inst_idx][prev_acc_pattern_idx].get();
        if (prev_node->getPathCost() == std::numeric_limits<int>::max()) {
          continue;
        }

        const int edge_cost = getEdgeCost(prev_node, curr_node, insts);
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

void FlexPA::genInstRowPatternCommit(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<frInst*>& insts)
{
  const bool is_debug_mode = false;

  const FlexDPNode* source_node = nodes[insts.size() + 1][0].get();
  const FlexDPNode* sink_node = nodes[insts.size()][0].get();

  FlexDPNode* curr_node = sink_node->getPrevNode();
  std::vector<int> inst_access_pattern_idx(insts.size(), -1);
  while (curr_node != source_node) {
    if (!curr_node) {
      std::string inst_names;
      for (frInst* inst : insts) {
        inst_names += '\n' + inst->getName();
      }
      logger_->error(DRT,
                     85,
                     "Valid access pattern combination not found for {}",
                     inst_names);
    }

    // non-virtual node
    auto [curr_inst_idx, curr_acc_patterns_idx] = curr_node->getIdx();
    inst_access_pattern_idx[curr_inst_idx] = curr_acc_patterns_idx;

    frInst* inst = insts[curr_inst_idx];
    int access_point_idx = 0;
    auto unique_class = unique_insts_.getUniqueClass(inst);
    auto access_pattern
        = unique_inst_patterns_.at(unique_class)[curr_acc_patterns_idx].get();
    auto& access_points = access_pattern->getPattern();

    // update inst_term ap
    for (auto& inst_term : inst->getInstTerms()) {
      if (isSkipInstTerm(inst_term.get())) {
        continue;
      }

      // to avoid unused variable warning in GCC
      for (int pin_idx = 0; pin_idx < inst_term->getTerm()->getPins().size();
           pin_idx++) {
        frAccessPoint* access_point = access_points[access_point_idx];
        inst_term->setAccessPoint(pin_idx, access_point);
        access_point_idx++;
      }
    }
    curr_node = curr_node->getPrevNode();
  }

  if (is_debug_mode) {
    genInstRowPatternPrint(nodes, insts);
  }
}

void FlexPA::genInstRowPatternPrint(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<frInst*>& insts)
{
  FlexDPNode* curr_node = nodes[insts.size()][0].get();
  int inst_cnt = insts.size();
  std::vector<int> inst_access_pattern_idx(insts.size(), -1);

  while (curr_node->hasPrevNode()) {
    // non-virtual node
    if (inst_cnt != (int) insts.size()) {
      auto [curr_inst_idx, curr_acc_pattern_idx] = curr_node->getIdx();
      inst_access_pattern_idx[curr_inst_idx] = curr_acc_pattern_idx;

      // print debug information
      auto& inst = insts[curr_inst_idx];
      int access_point_idx = 0;
      auto unique_class = unique_insts_.getUniqueClass(inst);
      auto access_pattern
          = unique_inst_patterns_.at(unique_class)[curr_acc_pattern_idx].get();
      auto& access_points = access_pattern->getPattern();

      for (auto& inst_term : inst->getInstTerms()) {
        if (isSkipInstTerm(inst_term.get())) {
          continue;
        }

        // for (auto &pin: inst_term->getTerm()->getPins()) {
        //  to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size());
             i++) {
          auto& access_point = access_points[access_point_idx];
          if (access_point) {
            const odb::Point& pt(access_point->getPoint());
            if (inst_term->hasNet()) {
              std::cout << " gcclean2via " << inst->getName() << " "
                        << inst_term->getTerm()->getName() << " "
                        << access_point->getViaDef()->getName() << " " << pt.x()
                        << " " << pt.y() << " " << inst->getOrient().getString()
                        << "\n";
              inst_term_valid_via_ap_cnt_++;
            }
          }
          access_point_idx++;
        }
      }
    }
    curr_node = curr_node->getPrevNode();
    inst_cnt--;
  }

  std::cout << std::flush;

  if (inst_cnt != -1) {
    logger_->error(DRT, 276, "Valid access pattern combination not found.");
  }
}

int FlexPA::getEdgeCost(FlexDPNode* prev_node,
                        FlexDPNode* curr_node,
                        const std::vector<frInst*>& insts)
{
  int edge_cost = 0;
  auto [prev_inst_idx, prev_acc_pattern_idx] = prev_node->getIdx();
  auto [curr_inst_idx, curr_acc_pattern_idx] = curr_node->getIdx();
  if (prev_node->isSource() || curr_node->isSink()) {
    return edge_cost;
  }

  // check DRC
  std::vector<std::unique_ptr<frVia>> temp_vias;
  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;
  // push the vias from prev inst access pattern and curr inst access pattern
  const auto prev_inst = insts[prev_inst_idx];
  const auto prev_unique_class = unique_insts_.getUniqueClass(prev_inst);
  const auto curr_inst = insts[curr_inst_idx];
  const auto curr_unique_class = unique_insts_.getUniqueClass(curr_inst);
  const auto prev_pin_access_pattern
      = unique_inst_patterns_.at(prev_unique_class)[prev_acc_pattern_idx].get();
  const auto curr_pin_access_pattern
      = unique_inst_patterns_.at(curr_unique_class)[curr_acc_pattern_idx].get();
  addAccessPatternObj(
      prev_inst, prev_pin_access_pattern, objs, temp_vias, true);
  addAccessPatternObj(
      curr_inst, curr_pin_access_pattern, objs, temp_vias, false);

  const bool has_vio = !genPatternsGC({prev_inst, curr_inst}, objs, Edge);
  if (!has_vio) {
    const int prev_node_cost = prev_node->getNodeCost();
    const int curr_node_cost = curr_node->getNodeCost();
    edge_cost = (prev_node_cost + curr_node_cost) / 2;
  } else {
    edge_cost = 1000;
  }

  return edge_cost;
}

void FlexPA::addAccessPatternObj(
    frInst* inst,
    FlexPinAccessPattern* access_pattern,
    std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
    std::vector<std::unique_ptr<frVia>>& vias,
    const bool isPrev)
{
  const odb::dbTransform xform = inst->getNoRotationTransform();
  int access_point_idx = 0;
  auto& access_points = access_pattern->getPattern();

  for (auto& inst_term : inst->getInstTerms()) {
    if (isSkipInstTerm(inst_term.get())) {
      continue;
    }

    // to avoid unused variable warning in GCC
    for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size()); i++) {
      auto& access_point = access_points[access_point_idx];
      if (!access_point
          || (isPrev && access_point != access_pattern->getBoundaryAP(false))) {
        access_point_idx++;
        continue;
      }
      if ((!isPrev) && access_point != access_pattern->getBoundaryAP(true)) {
        access_point_idx++;
        continue;
      }
      if (access_point->hasAccess(frDirEnum::U)) {
        odb::Point pt(access_point->getPoint());
        xform.apply(pt);
        auto via = std::make_unique<frVia>(access_point->getViaDef(), pt);
        auto rvia = via.get();
        if (inst_term->hasNet()) {
          objs.emplace_back(rvia, inst_term->getNet());
        } else {
          objs.emplace_back(rvia, inst_term.get());
        }
        vias.push_back(std::move(via));
      }
      access_point_idx++;
    }
  }
}

}  // namespace drt
