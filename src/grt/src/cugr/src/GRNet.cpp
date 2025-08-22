#include "GRNet.h"

#include <unordered_set>

namespace grt {

GRNet::GRNet(const CUGRNet& baseNet, Design* design, GridGraph* gridGraph)
{
  name_ = baseNet.getDbNet()->getName();
  int numPins = baseNet.getNumPins();
  pin_access_points_.resize(numPins);
  for (CUGRPin& pin : baseNet.getPins()) {
    std::vector<BoxOnLayer> pinShapes = pin.getPinShapes();
    std::unordered_set<uint64_t> included;
    for (const auto& pinShape : pinShapes) {
      BoxT<int> cells = gridGraph->rangeSearchCells(pinShape);
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

// void GRNet::getGuides(std::vector<std::pair<int, BoxT<int>>>& guides)
// const
// {
//     if (!routingTree) return;
//     GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
//         for (const auto& child : node->children) {
//             if (node->getLayerIdx() == child->getLayerIdx()) {
//                 guides.emplace_back(
//                     node->getLayerIdx(), BoxT<int>(
//                         std::min(node->x, child->x), std::min(node->y,
//                         child->y), std::max(node->x, child->x),
//                         std::max(node->y, child->y)
//                     )
//                 );
//             } else {
//                 int maxLayerIndex = std::max(node->getLayerIdx(),
//                 child->getLayerIdx()); for (int layerIdx =
//                 std::min(node->getLayerIdx(), child->getLayerIdx()); layerIdx <=
//                 maxLayerIndex; layerIdx++) {
//                     guides.emplace_back(layerIdx, BoxT<int>(node->x,
//                     node->y));
//                 }
//             }
//         }
//     });
// }

}  // namespace grt
