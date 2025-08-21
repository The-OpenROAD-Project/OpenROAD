#include "Design.h"

#include <unordered_map>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace grt {

Design::Design(odb::dbDatabase* db, utl::Logger* logger)
    : block_(db->getChip()->getBlock()), tech_(db->getTech()), logger_(logger)
{
  read();
  setUnitCosts();
}

void Design::read()
{
  libDBU = block_->getDbUnitsPerMicron();
  auto dieBound = block_->getDieArea();
  dieRegion = getBoxFromRect(dieBound);

  // 1. Get layers
  for (odb::dbTechLayer* tech_layer : tech_->getLayers()) {
    if (tech_layer->getType() == odb::dbTechLayerType::ROUTING) {
      odb::dbTrackGrid* track_grid = block_->findTrackGrid(tech_layer);
      if (track_grid != nullptr) {
        layers_.emplace_back(tech_layer, track_grid);
        if (tech_layer->getRoutingLevel() == 3) {
          int track_step, track_init, num_tracks;
          track_grid->getAverageTrackSpacing(
              track_step, track_init, num_tracks);
          defaultGridlineSpacing = track_step * 15;
        }
      }
    }
  }

  // 2. Get obstructions from insts
  for (odb::dbInst* db_inst : block_->getInsts()) {
    odb::dbTransform xform = db_inst->getTransform();

    for (odb::dbITerm* iterm : db_inst->getITerms()) {
      // get pin obstructions
      for (odb::dbMPin* mpin : iterm->getMTerm()->getMPins()) {
        for (odb::dbBox* box : mpin->getGeometry()) {
          odb::dbTechLayer* tech_layer = box->getTechLayer();
          if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
            continue;
          }

          int layerIndex = tech_layer->getRoutingLevel();
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
      if (box->getTechLayer()->getType() != odb::dbTechLayerType::ROUTING) {
        continue;
      }
      int layerIndex = box->getTechLayer()->getRoutingLevel();

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

  // 3. Populate netlist
  for (odb::dbNet* db_net : block_->getNets()) {
    if (db_net->isSpecial() || db_net->getSigType().isSupply()) {
      continue;
    }

    std::vector<CUGRPin> pins;
    int pin_count = 0;
    for (odb::dbBTerm* db_bterm : db_net->getBTerms()) {
      int x, y;
      std::vector<BoxOnLayer> pin_shapes;
      if (db_bterm->getFirstPinLocation(x, y)) {
        odb::Point position(x, y);
        auto bounds = db_bterm->getBBox();
        for (odb::dbBPin* bpin : db_bterm->getBPins()) {
          for (odb::dbBox* bpin_box : bpin->getBoxes()) {
            int layer_idx = bpin_box->getTechLayer()->getRoutingLevel();
            pin_shapes.emplace_back(layer_idx,
                                    getBoxFromRect(bpin_box->getBox()));
          }
        }
      }
      for (const auto& pin_shape : pin_shapes) {
        pins.emplace_back(pin_count, db_bterm, pin_shapes, true);
        pin_count++;
      }
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

          int layerIndex = tech_layer->getRoutingLevel();
          pin_shapes.emplace_back(
              layerIndex, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
        }
      }

      pins.emplace_back(pin_count, db_iterm, pin_shapes, false);
      pin_count++;
    }
    nets_.emplace_back(db_net, pins);
  }

  // 4. Get special nets as obstacles
  int numSpecialNets = 0;
  // for (odb::dbNet* specialNet : block_->getNets()) {
  //   if (!specialNet->isSpecial()) {
  //     continue;
  //   }
  //   numSpecialNets++;
  //   std::vector<odb::dbShape> via_boxes;
  //   for (odb::dbSWire* swire : specialNet->getSWires()) {
  //     for (odb::dbSBox* s : swire->getWires()) {
  //       if (s->isVia()) {
  //         s->getViaBoxes(via_boxes);
  //         for (const odb::dbShape& box : via_boxes) {
  //           odb::dbTechLayer* tech_layer = box.getTechLayer();
  //           if (tech_layer->getRoutingLevel() == 0) {
  //             continue;
  //           }
  //           odb::Rect via_rect = box.getBox();
  //           applyNetObstruction(via_rect, tech_layer, die_area, db_net);
  //         }
  //       } else {
  //         odb::Rect wire_rect = s->getBox();
  //         odb::dbTechLayer* tech_layer = s->getTechLayer();
  //         applyNetObstruction(wire_rect, tech_layer, die_area, db_net);
  //       }
  //     }
  //   }
  //   for (const DefWireDscp& wire : specialNet->getWire()) {
  //     for (const DefWireSegmentDscp& segment : wire.clsWireSegments) {
  //       int layerIdx
  //           = physicalDesign.getPhysicalLayerByName(segment.clsLayerName)
  //                 .getRelativeIndex();
  //       const int width = segment.clsRoutedWidth;
  //       odb::Point pos;
  //       int ext = 0;
  //       for (unsigned i = 0; i != segment.clsRoutingPoints.size(); ++i) {
  //         const DefRoutingPointDscp& pt = segment.clsRoutingPoints[i];
  //         const odb::Point& nextPos = pt.clsPos;
  //         const int nextExt = pt.clsHasExtension ? pt.clsExtension : 0;
  //         if (i >= 1) {
  //           for (unsigned dim = 0; dim != 2; ++dim) {
  //             if (pos[dim] == nextPos[dim]) {
  //               continue;
  //             }
  //             const int l = pos[dim] < nextPos[dim] ? pos[dim] - ext
  //                                                   : nextPos[dim] -
  //                                                   nextExt;
  //             const int h = pos[dim] < nextPos[dim] ? nextPos[dim] +
  //             nextExt
  //                                                   : pos[dim] + ext;
  //             BoxOnLayer box(layerIdx);
  //             box[dim].Set(l, h);
  //             box[1 - dim].Set(pos[1 - dim] - width / 2,
  //                              pos[1 - dim] + width / 2);
  //             obstacles_.emplace_back(box);
  //             break;
  //           }
  //         }
  //         pos = nextPos;
  //         ext = nextExt;
  //         if (!pt.clsHasVia) {
  //           continue;
  //         }
  //         const Rsyn::PhysicalVia& via
  //             = physicalDesign.getPhysicalViaByName(pt.clsViaName);
  //         const int botLayerIdx = via.getBottomLayer().getRelativeIndex();
  //         for (const Rsyn::PhysicalViaGeometry& geo :
  //              via.allBottomGeometries()) {
  //           Bounds bounds = geo.getBounds();
  //           bounds.translate(pos);
  //           const BoxOnLayer box(botLayerIdx, getBoxFromRect(bounds));
  //           obstacles_.emplace_back(box);
  //         }
  //         const int topLayerIdx = via.getTopLayer().getRelativeIndex();
  //         for (const Rsyn::PhysicalViaGeometry& geo :
  //         via.allTopGeometries())
  //         {
  //           Bounds bounds = geo.getBounds();
  //           bounds.translate(pos);
  //           const BoxOnLayer box(topLayerIdx, getBoxFromRect(bounds));
  //           obstacles_.emplace_back(box);
  //         }
  //         if (via.hasViaRule()) {
  //           const PointT<int> numRowCol
  //               = via.hasRowCol()
  //                     ? PointT<int>(via.getNumCols(), via.getNumRows())
  //                     : PointT<int>(1, 1);
  //           BoxOnLayer botBox(botLayerIdx);
  //           BoxOnLayer topBox(topLayerIdx);
  //           for (unsigned dimIdx = 0; dimIdx != 2; ++dimIdx) {
  //             const Dimension dim = static_cast<Dimension>(dimIdx);
  //             const int origin
  //                 = via.hasOrigin() ? pos[dim] + via.getOrigin(dim) :
  //                 pos[dim];
  //             const int botOff
  //                 = via.hasOffset()
  //                       ? origin + via.getOffset(Rsyn::BOTTOM_VIA_LEVEL,
  //                       dim) : origin;
  //             const int topOff
  //                 = via.hasOffset()
  //                       ? origin + via.getOffset(Rsyn::TOP_VIA_LEVEL, dim)
  //                       : origin;
  //             const int length = (via.getCutSize(dim) * numRowCol[dim]
  //                                 + via.getSpacing(dim) * (numRowCol[dim] -
  //                                 1))
  //                                / 2;
  //             const int botEnc
  //                 = length + via.getEnclosure(Rsyn::BOTTOM_VIA_LEVEL, dim);
  //             const int topEnc
  //                 = length + via.getEnclosure(Rsyn::TOP_VIA_LEVEL, dim);
  //             botBox[dim].Set(botOff - botEnc, botOff + botEnc);
  //             topBox[dim].Set(topOff - topEnc, topOff + topEnc);
  //           }
  //           obstacles_.emplace_back(botBox);
  //           obstacles_.emplace_back(topBox);
  //         }
  //         if (layerIdx == botLayerIdx) {
  //           layerIdx = topLayerIdx;
  //         } else if (layerIdx == topLayerIdx) {
  //           layerIdx = botLayerIdx;
  //         } else {
  //           logger_->report("Error: via " << pt.clsViaName << "of special
  //           net
  //           "
  //                                         << specialNet.getNet().clsName
  //                                         << " is on a wrong layer "
  //                                         << layerIdx);
  //           break;
  //         }
  //       }
  //     }
  //   }
  // }

  // 5. Get gridlines
  gridlines.resize(2);
  for (unsigned dimension = 0; dimension < 2; dimension++) {
    const int low = dieRegion[dimension].low;
    const int high = dieRegion[dimension].high;
    for (int i = low; i + defaultGridlineSpacing < high;
         i += defaultGridlineSpacing) {
      gridlines[dimension].push_back(i);
    }
    if (gridlines[dimension].back() != high) {
      gridlines[dimension].push_back(high);
    }
  }

  std::cout << "design statistics" << std::endl;
  std::cout << "lib DBU:             " << libDBU << std::endl;
  std::cout << "die region (in DBU): " << dieRegion << std::endl;
  std::cout << "num of nets :        " << nets_.size() << std::endl;
  std::cout << "num of special nets: " << numSpecialNets << std::endl;
  std::cout << "gcell grid:          " << gridlines[0].size() - 1 << " x "
            << gridlines[1].size() - 1 << " x " << getNumLayers() << std::endl;

  for (const auto& layer : layers_) {
    std::cout << "Layer " << layer.getName() << " statistics:" << std::endl;
    std::cout << "  Width: " << layer.getWidth() << std::endl;
    std::cout << "  Pitch: " << layer.getPitch() << std::endl;
    std::cout << "  Min Length: " << layer.getMinLength() << std::endl;
    std::cout << "  Default Spacing: " << layer.getDefaultSpacing()
              << std::endl;
    std::cout << "  Max EOL Spacing: " << layer.getMaxEolSpacing() << std::endl;
  }
}

void Design::setUnitCosts()
{
  int m2_pitch = layers_[1].getPitch();
  unit_length_wire_cost = weight_wire_length / m2_pitch;
  unit_via_cost = weight_via_number;
  unit_length_short_costs.resize(layers_.size());
  const CostT unit_area_short_cost = weight_short_area / (m2_pitch * m2_pitch);
  for (int layerIndex = 0; layerIndex < layers_.size(); layerIndex++) {
    unit_length_short_costs[layerIndex]
        = unit_area_short_cost * layers_[layerIndex].getWidth();
  }
}

void Design::getAllObstacles(std::vector<std::vector<BoxT<int>>>& allObstacles,
                             bool skipM1) const
{
  allObstacles.resize(getNumLayers());

  for (const BoxOnLayer& obs : obstacles_) {
    if (obs.layerIdx > 1 || !skipM1) {
      allObstacles[obs.layerIdx].emplace_back(obs.x, obs.y);
    }
  }

  int layer_idx = 0;
  for (auto layer_boxes : allObstacles) {
    std::cout << "Layer " << layer_idx << " has " << layer_boxes.size()
              << " obstacles." << std::endl;
    for (const auto& box : layer_boxes) {
      std::cout << "\t" << box << std::endl;
    }
    layer_idx++;
  }
}

}  // namespace grt
