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

GRNet::GRNet(const CUGRNet& baseNet, const GridGraph* gridGraph)
{
  index_ = baseNet.getIndex();
  db_net_ = baseNet.getDbNet();
  const int numPins = baseNet.getNumPins();
  pin_access_points_.resize(numPins);
  layer_range_ = baseNet.getLayerRange();
  slack_ = 0;
  is_critical_ = false;

  int pin_index = 0;
  for (CUGRPin& pin : baseNet.getPins()) {
    const std::vector<BoxOnLayer> pinShapes = pin.getPinShapes();
    std::unordered_set<uint64_t> included;
    for (const auto& pinShape : pinShapes) {
      const BoxT cells = gridGraph->rangeSearchCells(pinShape);
      for (int x = cells.lx(); x <= cells.hx(); x++) {
        for (int y = cells.ly(); y <= cells.hy(); y++) {
          const GRPoint point(pinShape.getLayerIdx(), x, y);
          const uint64_t hash = gridGraph->hashCell(point);
          if (included.find(hash) == included.end()) {
            pin_access_points_[pin.getIndex()].emplace_back(
                pinShape.getLayerIdx(), x, y);
            included.insert(hash);
          }
        }
      }
    }

    if (pin.isPort()) {
      pin_index_to_bterm_[pin_index] = pin.getBTerm();
    } else {
      pin_index_to_iterm_[pin_index] = pin.getITerm();
    }
    pin_index++;
  }

  for (const auto& accessPoints : pin_access_points_) {
    for (const auto& point : accessPoints) {
      bounding_box_.Update(point);
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
  bool is_local = true;
  PointT first_ap;

  if (!iterm_to_ap_.empty()) {
    first_ap = iterm_to_ap_.begin()->second.point;
  } else if (!bterm_to_ap_.empty()) {
    first_ap = bterm_to_ap_.begin()->second.point;
  } else {
    return true;
  }

  for (const auto& [_, ap] : iterm_to_ap_) {
    const PointT& ap_pos = ap.point;
    if (ap_pos != first_ap) {
      is_local = false;
    }
  }

  for (const auto& [_, ap] : bterm_to_ap_) {
    const PointT& ap_pos = ap.point;
    if (ap_pos != first_ap) {
      is_local = false;
    }
  }

  return is_local;
}

}  // namespace grt
