//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "via_repair.h"

#include <boost/polygon/polygon.hpp>
#include <set>

#include "grid.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"
#include "via.h"

namespace pdn {

ViaRepair::ViaRepair(utl::Logger* logger, const std::set<odb::dbNet*>& nets)
    : logger_(logger), nets_(nets)
{
}

void ViaRepair::repair()
{
  LayerViaTree vias = collectVias();
  removal_count_.clear();

  ObsRect combined_obs;
  if (use_obs_) {
    for (const auto& [layer, obs] :
         collectBlockObstructions((*nets_.begin())->getBlock())) {
      combined_obs[layer].insert(obs.begin(), obs.end());
    }
  }
  if (use_nets_) {
    for (const auto& [layer, obs] :
         collectInstanceObstructions((*nets_.begin())->getBlock())) {
      combined_obs[layer].insert(obs.begin(), obs.end());
    }
  }
  if (use_inst_) {
    for (const auto& [layer, obs] :
         collectNetObstructions((*nets_.begin())->getBlock())) {
      combined_obs[layer].insert(obs.begin(), obs.end());
    }
  }

  // find via violations
  using namespace boost::polygon::operators;
  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  std::map<odb::dbTechLayer*, std::set<odb::dbSBox*>> tech_vias_to_remove;
  std::map<odb::dbTechLayer*, std::set<odb::dbSBox*>> block_vias_to_remove;
  for (const auto& [layer, layer_obs] : combined_obs) {
    Polygon90Set layer_obstructions;
    for (const auto& obs : layer_obs) {
      std::array<Pt, 4> pts = {Pt(obs.xMin(), obs.yMin()),
                               Pt(obs.xMax(), obs.yMin()),
                               Pt(obs.xMax(), obs.yMax()),
                               Pt(obs.xMin(), obs.yMax())};

      Polygon90 poly;
      poly.set(pts.begin(), pts.end());
      layer_obstructions.insert(poly);
    }

    std::vector<Rectangle> layer_obstructions_rect;
    layer_obstructions.get_rectangles(layer_obstructions_rect);

    const auto& layer_vias = vias[layer];
    auto& tech_vias = tech_vias_to_remove[layer];
    auto& block_vias = block_vias_to_remove[layer];

    for (const auto& obs : layer_obstructions_rect) {
      const odb::Rect obs_rect(xl(obs), yl(obs), xh(obs), yh(obs));
      for (auto itr = layer_vias.qbegin(bgi::intersects(obs_rect));
           itr != layer_vias.qend();
           itr++) {
        odb::dbSBox* box = itr->second;
        if (box->getTechVia() != nullptr) {
          tech_vias.insert(box);
        } else {
          block_vias.insert(box);
        }
      }
    }
  }

  // delete offending vias
  for (const auto& [layer, vias] : tech_vias_to_remove) {
    auto& removed = removal_count_[layer];
    for (auto* via : vias) {
      removed++;
      odb::dbSBox::destroy(via);
    }
  }
  for (const auto& [layer, vias] : block_vias_to_remove) {
    if (!vias.empty()) {
      logger_->warn(
          utl::PDN,
          226,
          "{} contains block vias to be removed, which is not supported."),
          layer->getName();
    }
  }
}

ViaRepair::LayerViaTree ViaRepair::collectVias()
{
  LayerViaTree vias;
  via_count_.clear();

  // collect vias
  for (auto* net : nets_) {
    for (auto* swire : net->getSWires()) {
      for (auto* wire : swire->getWires()) {
        if (!wire->isVia()) {
          continue;
        }

        odb::dbTechLayer* cut_layer = nullptr;
        std::vector<odb::dbShape> via_boxes;
        wire->getViaBoxes(via_boxes);
        for (const auto& via_box : via_boxes) {
          auto* layer = via_box.getTechLayer();
          if (layer->getType() == odb::dbTechLayerType::CUT) {
            cut_layer = layer;
            via_count_[layer]++;
          }
        }

        auto* tech_via = wire->getTechVia();
        if (tech_via != nullptr) {
          int x, y;
          wire->getViaXY(x, y);
          for (const auto& obs : TechViaGenerator::getViaObstructionRects(
                   logger_, tech_via, x, y)) {
            vias[cut_layer].insert({obs, wire});
          }
        } else {
          // TODO: implement generate via
        }
      }
    }
  }

  return vias;
}

ViaRepair::ObsRect ViaRepair::collectBlockObstructions(odb::dbBlock* block)
{
  ObsRect obstructions;

  for (auto* obs : block->getObstructions()) {
    auto* box = obs->getBBox();
    if (box->getTechLayer()->getType() != odb::dbTechLayerType::CUT) {
      continue;
    }
    obstructions[box->getTechLayer()].insert(box->getBox());
  }

  return obstructions;
}

ViaRepair::ObsRect ViaRepair::collectNetObstructions(odb::dbBlock* block)
{
  ObsRect obstructions;

  for (auto* net : block->getNets()) {
    auto* wire = net->getWire();
    if (wire == nullptr) {
      continue;
    }

    odb::dbWireShapeItr itr;
    odb::dbShape shape;

    for (itr.begin(wire); itr.next(shape);) {
      if (!shape.isVia()) {
        continue;
      }

      std::vector<odb::dbShape> via_boxes;
      odb::dbShape::getViaBoxes(shape, via_boxes);
      for (const auto& via_box : via_boxes) {
        auto* layer = via_box.getTechLayer();
        if (layer->getType() != odb::dbTechLayerType::CUT) {
          continue;
        }
        obstructions[layer].insert(via_box.getBox());
      }
    }
  }

  return obstructions;
}

ViaRepair::ObsRect ViaRepair::collectInstanceObstructions(odb::dbBlock* block)
{
  ObsRect obstructions;

  for (auto* inst : block->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    const odb::dbTransform xform = inst->getTransform();

    odb::dbMaster* master = inst->getMaster();
    for (auto* obs : master->getObstructions()) {
      auto* layer = obs->getTechLayer();
      if (layer->getType() != odb::dbTechLayerType::CUT) {
        continue;
      }
      odb::Rect obs_rect = obs->getBox();
      xform.apply(obs_rect);
      obstructions[layer].insert(obs_rect);
    }

    for (const auto& [layer, shapes] : InstanceGrid::getInstancePins(inst)) {
      if (layer->getType() != odb::dbTechLayerType::CUT) {
        continue;
      }
      auto& layer_obs = obstructions[layer];
      for (const auto& shape : shapes) {
        layer_obs.insert(shape->getRect());
      }
    }
  }

  return obstructions;
}

void ViaRepair::report() const
{
  std::string nets;
  for (auto* net : nets_) {
    if (!nets.empty()) {
      nets += ", ";
    }
    nets += net->getName();
  }
  logger_->report("Via repair on {}", nets);
  bool removed_vias = false;
  for (const auto& [layer, removals] : removal_count_) {
    if (removals == 0) {
      continue;
    }
    const int total = via_count_.at(layer);
    double percent = static_cast<double>(removals) / total * 100;
    logger_->report("{} removed {} vias out of {} vias ({:.2f}%).",
                    layer->getName(),
                    removals,
                    total,
                    percent);
    removed_vias = true;
  }

  if (!removed_vias) {
    logger_->report("No vias removed.");
  }
}

}  // namespace pdn
