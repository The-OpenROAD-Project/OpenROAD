#include "GRNet.h"

#include <cstdint>
#include <unordered_set>
#include <vector>

#include "GRTree.h"
#include "GeoTypes.h"
#include "GridGraph.h"
#include "Netlist.h"
#include "geo.h"
#include "odb/db.h"

namespace grt {

GRNet::GRNet(const CUGRNet& base_net, const GridGraph* grid_graph)
{
  index_ = base_net.getIndex();
  db_net_ = base_net.getDbNet();
  const int num_pins = base_net.getNumPins();
  pin_access_points_.resize(num_pins);
  layer_range_ = base_net.getLayerRange();
  slack_ = 0;
  is_critical_ = false;

  for (CUGRPin& pin : base_net.getPins()) {
    const std::vector<BoxOnLayer> pin_shapes = pin.getPinShapes();
    std::unordered_set<uint64_t> included;
    for (const auto& pin_shape : pin_shapes) {
      const BoxT cells = grid_graph->rangeSearchCells(pin_shape);
      if (!cells.isValid()) {
        continue;
      }
      for (int x = cells.lx(); x <= cells.hx(); x++) {
        for (int y = cells.ly(); y <= cells.hy(); y++) {
          const GRPoint point(pin_shape.getLayerIdx(), x, y);
          const uint64_t hash = grid_graph->hashCell(point);
          if (included.find(hash) == included.end()) {
            pin_access_points_[pin.getIndex()].emplace_back(
                pin_shape.getLayerIdx(), x, y);
            included.insert(hash);
          }
        }
      }
    }

    if (pin.isPort()) {
      pin_index_to_bterm_[pin.getIndex()] = pin.getBTerm();
    } else {
      pin_index_to_iterm_[pin.getIndex()] = pin.getITerm();
    }
  }

  for (const auto& access_points : pin_access_points_) {
    for (const auto& point : access_points) {
      bounding_box_.update(point);
    }
  }
}

bool GRNet::isInsideLayerRange(int layer_index) const
{
  return layer_index >= layer_range_.min_layer
         && layer_index <= layer_range_.max_layer;
}

void GRNet::addPreferredAccessPoint(int pin_index, const AccessPoint& ap)
{
  if (auto it = pin_index_to_iterm_.find(pin_index);
      it != pin_index_to_iterm_.end()) {
    odb::dbITerm* iterm = it->second;
    iterm_to_ap_[iterm] = ap;
  } else if (auto it = pin_index_to_bterm_.find(pin_index);
             it != pin_index_to_bterm_.end()) {
    odb::dbBTerm* bterm = it->second;
    bterm_to_ap_[bterm] = ap;
  }
}

void GRNet::addBTermAccessPoint(odb::dbBTerm* bterm, const AccessPoint& ap)
{
  bterm_to_ap_[bterm] = ap;
}

void GRNet::addITermAccessPoint(odb::dbITerm* iterm, const AccessPoint& ap)
{
  iterm_to_ap_[iterm] = ap;
}

bool GRNet::isLocal() const
{
  PointT first_ap;

  if (!iterm_to_ap_.empty()) {
    first_ap = iterm_to_ap_.begin()->second.point;
  } else if (!bterm_to_ap_.empty()) {
    first_ap = bterm_to_ap_.begin()->second.point;
  } else {
    return true;
  }

  for (const auto& [_, ap] : iterm_to_ap_) {
    if (ap.point != first_ap) {
      return false;
    }
  }

  for (const auto& [_, ap] : bterm_to_ap_) {
    if (ap.point != first_ap) {
      return false;
    }
  }

  return true;
}

}  // namespace grt
