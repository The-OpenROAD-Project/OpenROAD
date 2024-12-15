/* Authors: Lutong Wang, Bangqi Xu*/
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

static inline void serializeInstRows(
    const std::vector<std::vector<frInst*>>& inst_rows,
    const std::string& file_name)
{
  paUpdate update;
  update.setInstRows(inst_rows);
  paUpdate::serialize(update, file_name);
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
        dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                            dst::JobMessage::UNICAST),
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
    dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                        dst::JobMessage::BROADCAST),
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
    int rowIdx = 0;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) inst_rows.size(); i++) {  // NOLINT
      try {
        auto& instRow = inst_rows[i];
        genInstRowPattern(instRow);
#pragma omp critical
        {
          rowIdx++;
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
  genInstRowPattern_commit(nodes, insts);
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
    const int unique_inst_idx = unique_insts_.getIndex(inst);
    auto& inst_patterns = unique_inst_patterns_[unique_inst_idx];
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

void FlexPA::genInstRowPattern_commit(
    std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
    const std::vector<frInst*>& insts)
{
  const bool is_debug_mode = false;
  FlexDPNode* curr_node = nodes[insts.size()][0].get();
  int inst_cnt = insts.size();
  std::vector<int> inst_access_pattern_idx(insts.size(), -1);
  while (curr_node->hasPrevNode()) {
    // non-virtual node
    if (inst_cnt != (int) insts.size()) {
      auto [curr_inst_idx, curr_acc_patterns_idx] = curr_node->getIdx();
      inst_access_pattern_idx[curr_inst_idx] = curr_acc_patterns_idx;

      auto& inst = insts[curr_inst_idx];
      int access_point_idx = 0;
      const int unique_inst_idx = unique_insts_.getIndex(inst);
      auto access_pattern
          = unique_inst_patterns_[unique_inst_idx][curr_acc_patterns_idx].get();
      auto& access_points = access_pattern->getPattern();

      // update inst_term ap
      for (auto& inst_term : inst->getInstTerms()) {
        if (isSkipInstTerm(inst_term.get())) {
          continue;
        }

        int pin_idx = 0;
        // to avoid unused variable warning in GCC
        for (int i = 0; i < (int) (inst_term->getTerm()->getPins().size());
             i++) {
          auto& access_point = access_points[access_point_idx];
          inst_term->setAccessPoint(pin_idx, access_point);
          pin_idx++;
          access_point_idx++;
        }
      }
    }
    curr_node = curr_node->getPrevNode();
    inst_cnt--;
  }

  if (inst_cnt != -1) {
    std::string inst_names;
    for (frInst* inst : insts) {
      inst_names += '\n' + inst->getName();
    }
    logger_->error(DRT,
                   85,
                   "Valid access pattern combination not found for {}",
                   inst_names);
  }

  if (is_debug_mode) {
    genInstRowPattern_print(nodes, insts);
  }
}

void FlexPA::genInstRowPattern_print(
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
      const int unique_inst_idx = unique_insts_.getIndex(inst);
      auto access_pattern
          = unique_inst_patterns_[unique_inst_idx][curr_acc_pattern_idx].get();
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
            const Point& pt(access_point->getPoint());
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
  const auto prev_unique_inst_idx = unique_insts_.getIndex(prev_inst);
  const auto curr_inst = insts[curr_inst_idx];
  const auto curr_unique_inst_idx = unique_insts_.getIndex(curr_inst);
  const auto prev_pin_access_pattern
      = unique_inst_patterns_[prev_unique_inst_idx][prev_acc_pattern_idx].get();
  const auto curr_pin_access_pattern
      = unique_inst_patterns_[curr_unique_inst_idx][curr_acc_pattern_idx].get();
  addAccessPatternObj(
      prev_inst, prev_pin_access_pattern, objs, temp_vias, true);
  addAccessPatternObj(
      curr_inst, curr_pin_access_pattern, objs, temp_vias, false);

  const bool has_vio = !genPatterns_gc({prev_inst, curr_inst}, objs, Edge);
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
  const dbTransform xform = inst->getNoRotationTransform();
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
        auto via = std::make_unique<frVia>(access_point->getViaDef());
        Point pt(access_point->getPoint());
        xform.apply(pt);
        via->setOrigin(pt);
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