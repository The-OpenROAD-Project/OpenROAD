#include "GRNet.h"

#include <cstdint>
#include <unordered_set>
#include <vector>

#include "GRTree.h"
#include "GeoTypes.h"
#include "GridGraph.h"
#include "Netlist.h"
#include "geo.h"

namespace grt {

GRNet::GRNet(const CUGRNet& baseNet, GridGraph* gridGraph)
{
  index_ = baseNet.getIndex();
  db_net_ = baseNet.getDbNet();
  int numPins = baseNet.getNumPins();
  pin_access_points_.resize(numPins);
  for (CUGRPin& pin : baseNet.getPins()) {
    std::vector<BoxOnLayer> pinShapes = pin.getPinShapes();
    std::unordered_set<uint64_t> included;
    for (const auto& pinShape : pinShapes) {
      BoxT cells = gridGraph->rangeSearchCells(pinShape);
      for (int x = cells.x.low; x <= cells.x.high; x++) {
        for (int y = cells.y.low; y <= cells.y.high; y++) {
          GRPoint point(pinShape.getLayerIdx(), x, y);
          uint64_t hash = gridGraph->hashCell(point);
          if (included.find(hash) == included.end()) {
            pin_access_points_[pin.getIndex()].emplace_back(
                pinShape.getLayerIdx(), x, y);
            included.insert(hash);
          }
        }
      }
    }
  }
  for (const auto& accessPoints : pin_access_points_) {
    for (const auto& point : accessPoints) {
      bounding_box_.Update(point);
    }
  }
}

}  // namespace grt
