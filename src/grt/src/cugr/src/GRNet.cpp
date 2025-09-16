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

GRNet::GRNet(const CUGRNet& baseNet, const GridGraph* gridGraph)
{
  index_ = baseNet.getIndex();
  db_net_ = baseNet.getDbNet();
  const int numPins = baseNet.getNumPins();
  pin_access_points_.resize(numPins);
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
  }
  for (const auto& accessPoints : pin_access_points_) {
    for (const auto& point : accessPoints) {
      bounding_box_.Update(point);
    }
  }
}

}  // namespace grt
