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
#include <cmath>

#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frVia.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
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
    if (origin.y() != prev_y_coord || origin.x() - prev_x_end_coord > router_cfg_->PA_ABUTMENT_EPSILON) {
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

  if (right_inst->getBoundaryBBox().xMin() - left_inst->getBoundaryBBox().xMax() > router_cfg_->PA_ABUTMENT_EPSILON ) {
    return false;
  }

  return true;
}

std::vector<frInst*> FlexPA::getAdjacentInstancesCluster(frInst* inst) const
{
  const auto inst_it = insts_set_.find(inst);
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
}

#define LAYER_NUM(str) (design_->getTech()->getLayer(str)->getLayerNum())

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
    auto& inst_patterns = unique_inst_patterns_[unique_class];

    // Calculating center of route guides of this inst 
    
    std::map<frInstTerm*, odb::Point> term_guide_centers; 

    if (router_cfg_->PA_RTGUIDE_MODE != 0) {
      for (auto& inst_term : inst->getInstTerms()) {
        if(isSkipInstTerm(inst_term.get()) || !inst_term->hasNet())
          continue;
        
        frNet* net = inst_term->getNet();
        std::vector<std::pair<odb::Rect, int>> guide_boxes; 

        for (auto& guide : net->getGuides()) {
          odb::Rect guide_box = guide->getBBox();
          int weight = 0; 

          std::map<int, int> weights{{LAYER_NUM("M1"), 10},
                                      {LAYER_NUM("M2"), 10},
                                      {LAYER_NUM("M3"), 8},
                                      {LAYER_NUM("M4"), 4},
                                      {LAYER_NUM("M5"), 2}};

          if (weights.find(guide->getBeginLayerNum()) != weights.end()) {
            weight = weights[guide->getBeginLayerNum()];
          } else {
            weight = 0;
          }
          guide_boxes.emplace_back(guide_box, weight);
        }

        std::sort(guide_boxes.begin(), guide_boxes.end(), [&inst_term](auto a, auto b) {
          return odb::Point::manhattanDistance(a.first.center(),
                                               inst_term->getBBox().center())
                 < odb::Point::manhattanDistance(b.first.center(),
                                                 inst_term->getBBox().center());
        });

        int xAvg = 0, yAvg = 0; 
        // --------------------------------------------------------
        // Mode 1: Center of all RTGuides
        // --------------------------------------------------------
        if (router_cfg_->PA_RTGUIDE_MODE == 1) {
          for (auto [bbox, weight] : guide_boxes) {
            xAvg += bbox.center().x();
            yAvg += bbox.center().y();
          }
  
          if (guide_boxes.empty()) {
            logger_->warn(DRT,
                          138,
                          "PA RTGuide Mode 1: No routeguide on {}",
                          net->getName());
            xAvg = inst_term->getBBox().center().x();
            yAvg = inst_term->getBBox().center().y();
          } else {
            xAvg /= static_cast<int>(guide_boxes.size());
            yAvg /= static_cast<int>(guide_boxes.size());
          }
        }

        // --------------------------------------------------------
        // Mode 2: Center of all RTGuides, weighted by layer.
        // --------------------------------------------------------
        else if (router_cfg_->PA_RTGUIDE_MODE == 2) {
          int sumWeight = 0;
          for (auto [bbox, weight] : guide_boxes) {
            xAvg += bbox.center().x() * weight;
            yAvg += bbox.center().y() * weight;
            sumWeight += weight;
          }
  
          if (sumWeight == 0) {
            xAvg = inst_term->getBBox().center().x();
            yAvg = inst_term->getBBox().center().y();
            logger_->warn(DRT,
                          145,
                          "PA RTGuide Mode 2: No routeguide or no in-range "
                          "routeguide on {}",
                          net->getName());
          } else {
            xAvg /= sumWeight;
            yAvg /= sumWeight;
          }
        } 
        
        // --------------------------------------------------------
        // Mode 3: Center of incident RTGuide
        // --------------------------------------------------------
        else if (router_cfg_->PA_RTGUIDE_MODE == 3) {
          if (guide_boxes.size() >= 1) {
            auto box = guide_boxes[0].first;
            xAvg = box.xCenter();
            yAvg = box.yCenter();
          } else {
            logger_->warn(DRT,
                          121,
                          "PA RTGuide Mode 3: No routeguide on {}",
                          net->getName());
            xAvg = inst_term->getBBox().center().x();
            yAvg = inst_term->getBBox().center().y();
          }
        } 

        // --------------------------------------------------------
        // Mode 4: Center of incident non-via RTGuide
        // --------------------------------------------------------
        else if (router_cfg_->PA_RTGUIDE_MODE == 4) {
          if (guide_boxes.size() >= 1) {
            odb::Rect firstNonVia = guide_boxes[0].first;
            for (auto [bbox, weight] : guide_boxes) {
              // An up-via is a square (1x1 gcell) guide segment that connects
              // from pin layer to the layer above. To estimate the first
              // non-up-via segment, we sort segments (see above) and then search
              // for the first non-square via is centered closest to the ITerm's
              // bbox
              if (bbox.dx() != bbox.dy()) {
                firstNonVia = bbox;
                break;
              }
            }
  
            xAvg = firstNonVia.xCenter();
            yAvg = firstNonVia.yCenter();
          } else {
            logger_->warn(DRT,
                          209,
                          "PA RTGuide Mode 4: No routeguide on {}",
                          net->getName());
            xAvg = inst_term->getBBox().center().x();
            yAvg = inst_term->getBBox().center().y();
          }
        } else if (router_cfg_->PA_RTGUIDE_MODE != 0) {
          logger_->warn(DRT,
                        111,
                        "PA RTGuide Usage Mode: {}",
                        router_cfg_->PA_RTGUIDE_MODE);
          throw std::out_of_range("Hacky: Invalid PA RTGuide Usage Mode.");
        }
        term_guide_centers[inst_term.get()] = odb::Point(xAvg, yAvg);
      }
    }

    // Find closest guide distance to normalize penalty

    std::map<frInstTerm*, double> term_min_dists;

    for (auto& inst_term : inst->getInstTerms()){ 
      if (isSkipInstTerm(inst_term.get()) || !term_guide_centers.count(inst_term.get())) continue; 

      odb::Point target = term_guide_centers[inst_term.get()]; 
      double min_d = std::numeric_limits<double>::max(); 

      for (auto& pin : inst_term->getTerm()->getPins()){ 
        uint pin_access_idx = inst->getPinAccessIdx(); 
        auto* pa = pin->getPinAccess(pin_access_idx); 
        for (auto& ap : pa->getAccessPoints()){ 
          double d = odb::Point::manhattanDistance(ap->getPoint(), target); 
          if (d < min_d) min_d = d; 
        }
      }

      term_min_dists[inst_term.get()] = std::max(min_d, 1.0); 
    }


    
    nodes[inst_idx]
        = std::vector<std::unique_ptr<FlexDPNode>>(inst_patterns.size());

    for (int acc_pattern_idx = 0; acc_pattern_idx < (int) inst_patterns.size();
         acc_pattern_idx++) {
      nodes[inst_idx][acc_pattern_idx] = std::make_unique<FlexDPNode>();
      auto access_pattern = inst_patterns[acc_pattern_idx].get();

      int access_pattern_cost = access_pattern ->getCost(); 

      const auto& access_points = access_pattern->getPattern(); 
      int ap_idx = 0; 

      for (auto& inst_term : inst->getInstTerms()) { 
        if (isSkipInstTerm(inst_term.get())) continue; 

        for (int pin_idx = 0; pin_idx < (int) inst_term->getTerm()->getPins().size(); pin_idx++){ 
          frAccessPoint* ap = access_points[ap_idx]; 
          ap_idx++; 

          if (!ap) continue; 

          // Cache lookups to avoid repeated map operations
          auto guide_center_it = term_guide_centers.find(inst_term.get());
          if (guide_center_it != term_guide_centers.end()){
            const odb::Point& guide_center = guide_center_it->second;
            odb::Point ap_loc = ap->getPoint(); 

            double dist = odb::Point::manhattanDistance(ap_loc, guide_center);
            auto min_dist_it = term_min_dists.find(inst_term.get());
            if (min_dist_it != term_min_dists.end()) {
              double min_dist = min_dist_it->second;
              // Penalty is proportional to how much further the AP is from the guide
              // compared to the minimum distance. Cap the ratio to avoid very large
              // penalties when min_dist is very small.
              double ratio = std::min(dist / min_dist, 10.0); // Cap at 10x
              double penalty = 5.0 * std::max(0.0, ratio - 1.0);

              access_pattern_cost += static_cast<int>(penalty);
            }
          }
        }
      }
      nodes[inst_idx][acc_pattern_idx]->setNodeCost(access_pattern_cost);
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
        = unique_inst_patterns_[unique_class][curr_acc_patterns_idx].get();
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
          = unique_inst_patterns_[unique_class][curr_acc_pattern_idx].get();
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
      = unique_inst_patterns_[prev_unique_class][prev_acc_pattern_idx].get();
  const auto curr_pin_access_pattern
      = unique_inst_patterns_[curr_unique_class][curr_acc_pattern_idx].get();
  addAccessPatternObj(
      prev_inst, prev_pin_access_pattern, objs, temp_vias, true);
  addAccessPatternObj(
      curr_inst, curr_pin_access_pattern, objs, temp_vias, false);

  const bool has_vio = !genPatternsGC({prev_inst, curr_inst}, objs, Edge);
  if (!has_vio) {
    const int prev_node_cost = prev_node->getNodeCost();
    const int curr_node_cost = curr_node->getNodeCost();
    
    edge_cost = (prev_node_cost + curr_node_cost) / 2;

    // If non-zero epsilon and if two cells are not abutting, 
    // then we wish to apply a large cost if two boundary APs are sharing a track and a weak one if they are one track apart.
    if(router_cfg_->PA_ABUTMENT_EPSILON > 0 && prev_inst->getBBox().xMax() != curr_inst->getBBox().xMin()) {
      const frAccessPoint* prev_boundary_ap = prev_pin_access_pattern->getBoundaryAP(false);
      const frAccessPoint* curr_boundary_ap = prev_pin_access_pattern->getBoundaryAP(true);
      
      if(prev_boundary_ap->y() == curr_boundary_ap->y()) {
        edge_cost = 1000;
      } else if (std::abs(prev_boundary_ap->y() - curr_boundary_ap->y()) <= 36) {
        edge_cost *= 2;
      }
    }
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
