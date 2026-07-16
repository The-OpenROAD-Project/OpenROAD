#include "Design.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "CUGR.h"
#include "GeoTypes.h"
#include "Layers.h"
#include "Netlist.h"
#include "db_sta/dbSta.hh"
#include "odb/PtrSetMap.h"
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
               const int max_routing_layer,
               const odb::PtrSet<odb::dbNet>& clock_nets,
               const bool verbose)
    : block_(db->getChip()->getBlock()),
      tech_(db->getTech()),
      logger_(logger),
      constants_(constants),
      min_routing_layer_(min_routing_layer),
      max_routing_layer_(max_routing_layer),
      clock_nets_(clock_nets),
      verbose_(verbose)
{
  read();
  setUnitCosts();
}

void Design::read()
{
  lib_dbu_ = block_->getDbUnitsPerMicron();
  const odb::Rect die_bound = block_->getDieArea();
  die_region_ = getBoxFromRect(die_bound);

  readLayers();

  readNetlist();

  readInstanceObstructions();

  const int num_special_nets = readSpecialNetObstructions();

  readDesignObstructions();

  computeGrid();

  computeViaDemandLengths();

  if (verbose_) {
    logger_->report("Design statistics");
    logger_->report("Nets:                {}", nets_.size());
    logger_->report("Special nets:        {}", num_special_nets);
    logger_->report("Routing layers:      {}", getNumLayers());
  }
}

void Design::readLayers()
{
  for (odb::dbTechLayer* tech_layer : tech_->getLayers()) {
    if (tech_layer->getType() == odb::dbTechLayerType::ROUTING
        && tech_layer->getRoutingLevel() <= max_routing_layer_) {
      odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
      if (track_grid != nullptr) {
        layers_.emplace_back(tech_layer, track_grid);
      }
    }
  }
  default_gridline_spacing_ = block_->getGCellTileSize();
}

void Design::readNetlist()
{
  int net_index = 0;
  for (odb::dbNet* db_net : block_->getNets()) {
    if (db_net->isSpecial() || db_net->getSigType().isSupply()
        || !db_net->getSWires().empty() || db_net->isConnectedByAbutment()) {
      continue;
    }

    auto pins = makeNetPins(db_net);
    if (pins.size() < 2) {
      continue;
    }

    LayerRange layer_range = {.min_layer = min_routing_layer_ - 1,
                              .max_layer = max_routing_layer_ - 1};
    const int min_clk_layer = block_->getMinLayerForClock();
    const int max_clk_layer = block_->getMaxLayerForClock();
    if (clock_nets_.contains(db_net) && min_clk_layer > 0
        && max_clk_layer > 0) {
      layer_range.min_layer = min_clk_layer - 1;
      layer_range.max_layer = max_clk_layer - 1;
    }

    nets_.emplace_back(net_index, db_net, pins, layer_range);
    db_net_to_id_[db_net] = net_index;
    net_index++;
  }
}

std::vector<CUGRPin> Design::makeNetPins(odb::dbNet* db_net)
{
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

    pins.emplace_back(pin_count, db_bterm, pin_shapes);
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

        int layer_index = tech_layer->getRoutingLevel() - 1;
        pin_shapes.emplace_back(
            layer_index, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
      }
    }

    pins.emplace_back(pin_count, db_iterm, pin_shapes);
    pin_count++;
  }

  return pins;
}

int Design::updateNet(odb::dbNet* db_net)
{
  if (db_net->isSpecial() || db_net->getSigType().isSupply()
      || !db_net->getSWires().empty() || db_net->isConnectedByAbutment()) {
    return -1;
  }

  auto pins = makeNetPins(db_net);

  LayerRange layer_range = {.min_layer = min_routing_layer_ - 1,
                            .max_layer = max_routing_layer_ - 1};
  const int min_clk_layer = block_->getMinLayerForClock();
  const int max_clk_layer = block_->getMaxLayerForClock();
  if (clock_nets_.contains(db_net) && min_clk_layer > 0 && max_clk_layer > 0) {
    layer_range.min_layer = min_clk_layer - 1;
    layer_range.max_layer = max_clk_layer - 1;
  }

  auto it = db_net_to_id_.find(db_net);
  if (it != db_net_to_id_.end()) {
    nets_[it->second].setPins(std::move(pins));
    nets_[it->second].setLayerRange(layer_range);
    return it->second;
  }

  if (pins.size() < 2) {
    return -1;
  }

  const int net_index = static_cast<int>(nets_.size());
  db_net_to_id_[db_net] = net_index;
  nets_.emplace_back(net_index, db_net, std::move(pins), layer_range);
  return net_index;
}

void Design::removeNet(odb::dbNet* db_net)
{
  auto it = db_net_to_id_.find(db_net);
  if (it != db_net_to_id_.end()) {
    nets_[it->second].invalidate();
    db_net_to_id_.erase(it);
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

          int layer_index = tech_layer->getRoutingLevel() - 1;
          odb::Rect rect = box->getBox();
          xform.apply(rect);

          BoxOnLayer box_on_layer(
              layer_index, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
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

      int layer_index = tech_layer->getRoutingLevel() - 1;
      odb::Rect rect = box->getBox();
      xform.apply(rect);

      odb::Point lower_bound = odb::Point(rect.xMin(), rect.yMin());
      odb::Point upper_bound = odb::Point(rect.xMax(), rect.yMax());
      odb::Rect obstruction_rect = odb::Rect(lower_bound, upper_bound);
      obstacles_.emplace_back(layer_index,
                              obstruction_rect.xMin(),
                              obstruction_rect.yMin(),
                              obstruction_rect.xMax(),
                              obstruction_rect.yMax());
    }
  }
}

int Design::readSpecialNetObstructions()
{
  int num_special_nets = 0;
  for (odb::dbNet* db_net : block_->getNets()) {
    if (!db_net->isSpecial() && !db_net->getSigType().isSupply()) {
      continue;
    }

    uint32_t wire_cnt = 0;
    uint32_t via_cnt = 0;
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
  return num_special_nets;
}

void Design::readDesignObstructions()
{
  for (odb::dbObstruction* obstruction : block_->getObstructions()) {
    odb::dbBox* box = obstruction->getBBox();
    odb::dbTechLayer* tech_layer = box->getTechLayer();
    if (tech_layer == nullptr
        || tech_layer->getType() != odb::dbTechLayerType::ROUTING
        || tech_layer->getRoutingLevel() > max_routing_layer_) {
      continue;
    }

    int layer_index = tech_layer->getRoutingLevel() - 1;
    odb::Rect rect = box->getBox();

    BoxOnLayer box_on_layer(
        layer_index, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
    obstacles_.push_back(box_on_layer);
  }
}

void Design::computeGrid()
{
  gridlines_.resize(2);
  for (int dimension = 0; dimension < 2; dimension++) {
    const int low = die_region_[dimension].low();
    const int high = die_region_[dimension].high();
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
  const int m2_pitch = layers_[1].getPitch();
  unit_length_wire_cost_ = constants_.weight_wire_length / m2_pitch;
  unit_via_cost_ = constants_.weight_via_number;
  unit_length_short_costs_.resize(layers_.size());
  const CostT unit_area_short_cost
      = constants_.weight_short_area / (m2_pitch * m2_pitch);
  for (int layer_index = 0; layer_index < layers_.size(); layer_index++) {
    unit_length_short_costs_[layer_index]
        = unit_area_short_cost * layers_[layer_index].getWidth();
  }
}

odb::dbTechVia* Design::chooseViaForPair(odb::dbTechLayer* lower_tl,
                                         odb::dbTechLayer* upper_tl) const
{
  // Rank vias connecting the pair by a drt-like priority: OR_DEFAULT, then
  // fewest cuts, then LEF-default, then smallest enclosure.
  odb::dbTechLayer* cut_tl = lower_tl->getUpperLayer();
  odb::dbTechVia* best = nullptr;
  std::tuple<bool, int, bool, int64_t> best_key;
  for (odb::dbTechVia* via : tech_->getVias()) {
    if (via->getBottomLayer() != lower_tl || via->getTopLayer() != upper_tl) {
      continue;
    }
    int cuts = 0;
    int64_t enc_area = 0;
    for (odb::dbBox* box : via->getBoxes()) {
      odb::dbTechLayer* bl = box->getTechLayer();
      if (bl == cut_tl) {
        cuts++;
      } else if (bl == lower_tl || bl == upper_tl) {
        enc_area += box->getBox().area();
      }
    }
    const bool or_default
        = odb::dbStringProperty::find(via, "OR_DEFAULT") != nullptr;
    const std::tuple<bool, int, bool, int64_t> key{
        !or_default, cuts, !via->isDefault(), enc_area};
    if (best == nullptr || key < best_key) {
      best = via;
      best_key = key;
    }
  }
  return best;
}

void Design::computeViaDemandLengths()
{
  const int num_layers = getNumLayers();
  // Fallback proxy (min-area stub x via_multiplier) for pairs with no via.
  via_demand_length_lower_.assign(num_layers, 0.0);
  via_demand_length_upper_.assign(num_layers, 0.0);

  const bool debug = logger_->debugCheck(utl::GRT, "via_geom", 1);
  int fallback_pairs = 0;
  for (int i = 0; i + 1 < num_layers; i++) {
    const MetalLayer& lower = layers_[i];
    const MetalLayer& upper = layers_[i + 1];
    odb::dbTechLayer* lower_tl = lower.getTechLayer();
    odb::dbTechLayer* upper_tl = upper.getTechLayer();
    odb::dbTechVia* via = chooseViaForPair(lower_tl, upper_tl);
    if (via == nullptr) {
      fallback_pairs++;
    }

    double num_lower = lower.getMinLength() * constants_.via_multiplier;
    double num_upper = upper.getMinLength() * constants_.via_multiplier;
    // Union the boxes on each layer; a via may have several rects per layer.
    odb::Rect lo_box, up_box;
    lo_box.mergeInit();
    up_box.mergeInit();
    if (via != nullptr) {
      for (odb::dbBox* box : via->getBoxes()) {
        if (box->getTechLayer() == lower_tl) {
          lo_box.merge(box->getBox());
        } else if (box->getTechLayer() == upper_tl) {
          up_box.merge(box->getBox());
        }
      }
      if (!lo_box.isInverted() && lo_box.dx() > 0 && lo_box.dy() > 0) {
        num_lower = viaDemandLength(lower, lo_box.dx(), lo_box.dy());
      }
      if (!up_box.isInverted() && up_box.dx() > 0 && up_box.dy() > 0) {
        num_upper = viaDemandLength(upper, up_box.dx(), up_box.dy());
      }
    }
    via_demand_length_lower_[i] = num_lower;
    via_demand_length_upper_[i] = num_upper;

    if (debug) {
      // Report enclosures, lengths, and per-track demand for each pair.
      const int gcell = default_gridline_spacing_;
      const std::string via_src
          = via != nullptr ? via->getName() : std::string("min_area-fallback");
      debugPrint(
          logger_,
          utl::GRT,
          "via_geom",
          1,
          "via {}->{} [{}]: encl lower={}x{} upper={}x{} -> len "
          "lower={:.1f} upper={:.1f} -> demand lower={:.4f} upper={:.4f}",
          lower.getName(),
          upper.getName(),
          via_src,
          lo_box.isInverted() ? 0 : lo_box.dx(),
          lo_box.isInverted() ? 0 : lo_box.dy(),
          up_box.isInverted() ? 0 : up_box.dx(),
          up_box.isInverted() ? 0 : up_box.dy(),
          num_lower,
          num_upper,
          gcell > 0 ? num_lower / gcell : 0.0,
          gcell > 0 ? num_upper / gcell : 0.0);
    }
  }

  if (fallback_pairs > 0) {
    logger_->warn(utl::GRT,
                  173,
                  "{} layer pair(s) have no via; using min-area via demand.",
                  fallback_pairs);
  }
}

double Design::viaDemandLength(const MetalLayer& layer,
                               const int dx,
                               const int dy) const
{
  const int pitch = layer.getPitch();
  // getSpacing() is 0 on parallel-table-only techs; use default spacing then.
  const int spacing
      = layer.getSpacing() > 0 ? layer.getSpacing() : layer.getDefaultSpacing();
  // Split the pad into extent along the routing direction and across tracks.
  const int along = (layer.getDirection() == MetalLayer::H) ? dx : dy;
  const int perp = (layer.getDirection() == MetalLayer::H) ? dy : dx;
  // A via blocks whole tracks; ceil the keep-out to an integer track count.
  const double tracks_blocked
      = pitch > 0 ? std::ceil(static_cast<double>(perp + 2 * spacing) / pitch)
                  : 1.0;
  return along * tracks_blocked;
}

void Design::getAllObstacles(std::vector<std::vector<BoxT>>& all_obstacles,
                             const bool skip_m1) const
{
  all_obstacles.resize(getNumLayers());

  for (const BoxOnLayer& obs : obstacles_) {
    if (obs.getLayerIdx() > 0 || !skip_m1) {
      all_obstacles[obs.getLayerIdx()].emplace_back(obs.x(), obs.y());
    }
  }
}

void Design::printNets() const
{
  for (const CUGRNet& net : nets_) {
    if (!net.isValid()) {
      continue;
    }
    logger_->report("Net: {}", net.getName());
    for (const auto& pin : net.getPins()) {
      logger_->report("\tPin: {}", pin.getName());
      for (const auto& box : pin.getPinShapes()) {
        logger_->report("\t\t{}", box);
      }
      logger_->report("");
    }
  }
}

void Design::printBlockages() const
{
  std::vector<std::vector<BoxT>> all_obstacles;
  getAllObstacles(all_obstacles, true);
  logger_->report("design obstacles: {}", all_obstacles.size());
  for (int i = 0; i < all_obstacles.size(); i++) {
    logger_->report("obs in layer {}: {}", (i + 1), all_obstacles[i].size());
    for (const auto& obstacle : all_obstacles[i]) {
      logger_->report("  Obstacle on layer {}: {}",
                      (i + 1),  // adjust to 1-based layer index
                      obstacle);
    }
  }
}

}  // namespace grt