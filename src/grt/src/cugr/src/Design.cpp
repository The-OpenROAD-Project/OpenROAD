#include "Design.h"

#include <iostream>
#include <vector>

#include "CUGR.h"
#include "GeoTypes.h"
#include "Netlist.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace grt {

Design::Design(odb::dbDatabase* db,
               utl::Logger* logger,
               const Constants& constants,
               const int min_routing_layer,
               const int max_routing_layer)
    : block_(db->getChip()->getBlock()),
      tech_(db->getTech()),
      logger_(logger),
      constants_(constants),
      min_routing_layer_(min_routing_layer),
      max_routing_layer_(max_routing_layer)
{
  read();
  setUnitCosts();
}

void Design::read()
{
  lib_dbu_ = block_->getDbUnitsPerMicron();
  auto dieBound = block_->getDieArea();
  die_region_ = getBoxFromRect(dieBound);

  readLayers();

  readNetlist();

  readInstanceObstructions();

  int num_special_nets = 0;
  readSpecialNetObstructions(num_special_nets);

  computeGrid();

  std::cout << "design statistics\n";
  std::cout << "lib DBU:             " << lib_dbu_ << '\n';
  std::cout << "die region (in DBU): " << die_region_ << '\n';
  std::cout << "num of nets :        " << nets_.size() << '\n';
  std::cout << "num of special nets: " << num_special_nets << '\n';
  std::cout << "gcell grid:          " << gridlines_[0].size() - 1 << " x "
            << gridlines_[1].size() - 1 << " x " << getNumLayers() << '\n';
}

void Design::readLayers()
{
  for (odb::dbTechLayer* tech_layer : tech_->getLayers()) {
    if (tech_layer->getType() == odb::dbTechLayerType::ROUTING
        && tech_layer->getRoutingLevel() <= max_routing_layer_) {
      odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
      if (track_grid != nullptr) {
        layers_.emplace_back(tech_layer, track_grid);
        if (tech_layer->getRoutingLevel() == 3) {
          int track_step, track_init, num_tracks;
          track_grid->getAverageTrackSpacing(
              track_step, track_init, num_tracks);
          default_gridline_spacing_ = track_step * 15;
        }
      }
    }
  }
}

void Design::readNetlist()
{
  int net_index = 0;
  for (odb::dbNet* db_net : block_->getNets()) {
    if (db_net->isSpecial() || db_net->getSigType().isSupply()
        || !db_net->getSWires().empty() || db_net->isConnectedByAbutment()) {
      continue;
    }

    std::vector<CUGRPin> pins;
    int pin_count = 0;
    for (odb::dbBTerm* db_bterm : db_net->getBTerms()) {
      int x, y;
      std::vector<BoxOnLayer> pin_shapes;
      if (db_bterm->getFirstPinLocation(x, y)) {
        odb::Point position(x, y);
        for (odb::dbBPin* bpin : db_bterm->getBPins()) {
          for (odb::dbBox* bpin_box : bpin->getBoxes()) {
            // adjust layer idx to start with zero
            int layer_idx = bpin_box->getTechLayer()->getRoutingLevel() - 1;
            pin_shapes.emplace_back(layer_idx,
                                    getBoxFromRect(bpin_box->getBox()));
          }
        }
      }

      pins.emplace_back(pin_count, db_bterm, pin_shapes, true);
      pin_count++;
    }

    for (odb::dbITerm* db_iterm : db_net->getITerms()) {
      std::vector<BoxOnLayer> pin_shapes;
      odb::dbTransform xform = db_iterm->getInst()->getTransform();
      for (odb::dbMPin* mpin : db_iterm->getMTerm()->getMPins()) {
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
            continue;
          }

          odb::Rect rect = box->getBox();
          xform.apply(rect);

          int layerIndex = tech_layer->getRoutingLevel() - 1;
          pin_shapes.emplace_back(
              layerIndex, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
        }
      }

      pins.emplace_back(pin_count, db_iterm, pin_shapes, false);
      pin_count++;
    }
    nets_.emplace_back(net_index, db_net, pins);
    net_index++;
  }
}

void Design::readInstanceObstructions()
{
  for (odb::dbInst* db_inst : block_->getInsts()) {
    odb::dbTransform xform = db_inst->getTransform();

    for (odb::dbITerm* iterm : db_inst->getITerms()) {
      // get pin obstructions
      for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer == nullptr
              || tech_layer->getType() != odb::dbTechLayerType::ROUTING
              || tech_layer->getRoutingLevel() > max_routing_layer_) {
            continue;
          }

          int layerIndex = tech_layer->getRoutingLevel() - 1;
          odb::Rect rect = box->getBox();
          xform.apply(rect);

          BoxOnLayer box_on_layer(
              layerIndex, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
          obstacles_.push_back(box_on_layer);
        }
      }
    }

    // get lib obstructions
    for (odb::dbBox* box : db_inst->getMaster()->getObstructions()) {
      odb::dbTechLayer* tech_layer = box->getTechLayer();
      if (tech_layer == nullptr
          || tech_layer->getType() != odb::dbTechLayerType::ROUTING
          || tech_layer->getRoutingLevel() > max_routing_layer_) {
        continue;
      }

      int layerIndex = tech_layer->getRoutingLevel() - 1;
      odb::Rect rect = box->getBox();
      xform.apply(rect);

      odb::Point lower_bound = odb::Point(rect.xMin(), rect.yMin());
      odb::Point upper_bound = odb::Point(rect.xMax(), rect.yMax());
      odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
      obstacles_.emplace_back(layerIndex,
                              obstruction_rect.xMin(),
                              obstruction_rect.yMin(),
                              obstruction_rect.xMax(),
                              obstruction_rect.yMax());
    }
  }
}

void Design::readSpecialNetObstructions(int& num_special_nets)
{
  for (odb::dbNet* db_net : block_->getNets()) {
    if (!db_net->isSpecial() && !db_net->getSigType().isSupply()) {
      continue;
    }

    odb::uint wire_cnt = 0, via_cnt = 0;
    db_net->getWireCount(wire_cnt, via_cnt);
    if (wire_cnt == 0) {
      continue;
    }

    std::vector<odb::dbShape> via_boxes;
    for (odb::dbSWire* swire : db_net->getSWires()) {
      for (odb::dbSBox* s : swire->getWires()) {
        if (s->isVia()) {
          s->getViaBoxes(via_boxes);
          for (const odb::dbShape& box : via_boxes) {
            odb::dbTechLayer* tech_layer = box.getTechLayer();
            if (tech_layer->getRoutingLevel() == 0
                || tech_layer->getRoutingLevel() > max_routing_layer_) {
              continue;
            }
            odb::Rect via_rect = box.getBox();
            obstacles_.emplace_back(tech_layer->getRoutingLevel() - 1,
                                    via_rect.xMin(),
                                    via_rect.yMin(),
                                    via_rect.xMax(),
                                    via_rect.yMax());
          }
        } else {
          odb::dbTechLayer* tech_layer = s->getTechLayer();
          if (tech_layer->getRoutingLevel() <= max_routing_layer_) {
            odb::Rect wire_rect = s->getBox();
            obstacles_.emplace_back(tech_layer->getRoutingLevel() - 1,
                                    wire_rect.xMin(),
                                    wire_rect.yMin(),
                                    wire_rect.xMax(),
                                    wire_rect.yMax());
          }
        }
      }
    }

    num_special_nets++;
  }
}

void Design::computeGrid()
{
  gridlines_.resize(2);
  for (unsigned dimension = 0; dimension < 2; dimension++) {
    const int low = die_region_[dimension].low;
    const int high = die_region_[dimension].high;
    for (int i = low; i + default_gridline_spacing_ < high;
         i += default_gridline_spacing_) {
      gridlines_[dimension].push_back(i);
    }
    if (gridlines_[dimension].back() != high) {
      gridlines_[dimension].push_back(high);
    }
  }
}

void Design::setUnitCosts()
{
  int m2_pitch = layers_[1].getPitch();
  unit_length_wire_cost_ = constants_.weight_wire_length / m2_pitch;
  unit_via_cost_ = constants_.weight_via_number;
  unit_length_short_costs_.resize(layers_.size());
  const CostT unit_area_short_cost
      = constants_.weight_short_area / (m2_pitch * m2_pitch);
  for (int layerIndex = 0; layerIndex < layers_.size(); layerIndex++) {
    unit_length_short_costs_[layerIndex]
        = unit_area_short_cost * layers_[layerIndex].getWidth();
  }
}

void Design::getAllObstacles(std::vector<std::vector<BoxT>>& all_obstacles,
                             bool skip_m1) const
{
  all_obstacles.resize(getNumLayers());

  for (const BoxOnLayer& obs : obstacles_) {
    if (obs.getLayerIdx() > 0 || !skip_m1) {
      all_obstacles[obs.getLayerIdx()].emplace_back(obs.x, obs.y);
    }
  }
}

void Design::printNets() const
{
  for (const CUGRNet& net : nets_) {
    std::cout << "Net: " << net.getName() << '\n';
    for (const auto& pin : net.getPins()) {
      std::cout << "\tPin: " << pin.getName() << "\n";
      for (const auto& box : pin.getPinShapes()) {
        std::cout << "\t\t" << box << "\n";  // adjust to 1-based
      }
      std::cout << '\n';
    }
  }
}

void Design::printBlockages() const
{
  std::vector<std::vector<BoxT>> all_obstacles;
  getAllObstacles(all_obstacles, true);
  std::cout << "design obstacles: " << all_obstacles.size() << '\n';
  for (int i = 0; i < all_obstacles.size(); i++) {
    std::cout << "obs in layer " << (i + 1) << ": " << all_obstacles[i].size()
              << '\n';
    for (const auto& obstacle : all_obstacles[i]) {
      std::cout << "  Obstacle on layer "
                << (i + 1)  // adjust to 1-based layer index
                << ": " << obstacle << '\n';
    }
  }
}

}  // namespace grt
