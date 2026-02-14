// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "io/GuideProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frPoint.h"
#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGCellPattern.h"
#include "db/tech/frLayer.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "global.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using odb::dbTechLayerDir;

namespace drt::io {
using Interval = boost::icl::interval<frCoord>;
namespace {
/**
 * @brief Returns the closest point on the perimeter of the rectangle r to the
 * point p
 */
odb::Point getClosestPoint(const frRect& r, const odb::Point& p)
{
  const odb::Rect b = r.getBBox();
  const int x = std::clamp(p.getX(), b.xMin(), b.xMax());
  const int y = std::clamp(p.getY(), b.yMin(), b.yMax());
  return odb::Point(x, y);
}
/**
 * @brief Returns the shapes of the given pin on all layers.
 * @note Assumes pin's typeId() is either frcBTerm or frcInstTerm
 */
std::vector<frRect> getPinShapes(const frBlockObject* pin)
{
  std::vector<frRect> pinShapes;
  if (pin->typeId() == frcBTerm) {
    static_cast<const frBTerm*>(pin)->getShapes(pinShapes);
  } else {
    static_cast<const frInstTerm*>(pin)->getShapes(pinShapes);
  }
  return pinShapes;
}

/**
 * @brief Returns name of the given pin.
 * @note Assumes pin's typeId() is either frcBTerm or frcInstTerm
 */
std::string getPinName(const frBlockObject* pin)
{
  if (pin->typeId() == frcBTerm) {
    return static_cast<const frBTerm*>(pin)->getName();
  }
  return static_cast<const frInstTerm*>(pin)->getName();
}
/**
 * Returns the preferred access point for required pin.
 *
 * The function returns the preferred acces point of the iterm from the
 * required pin index. If there is no preferred access point set for that pin,
 * then the first access point for that pin is returned.
 *
 * @param iterm The ITerm we are considering.
 * @param pin_idx The pin index of the pin we are considering.
 * @returns The preferred access point if found, and nullptr otherwise.
 */
frAccessPoint* getPrefAp(const frInstTerm* iterm, const int pin_idx)
{
  const int pin_access_idx = iterm->getInst()->getPinAccessIdx();
  const auto pin = iterm->getTerm()->getPins().at(pin_idx).get();
  if (!pin->hasPinAccess()) {
    return nullptr;
  }
  frAccessPoint* pref_ap = (iterm->getAccessPoints())[pin_idx];

  if (!pref_ap) {
    auto pa = pin->getPinAccess(pin_access_idx);
    if (pa->getNumAccessPoints() != 0) {
      pref_ap = pin->getPinAccess(pin_access_idx)->getAccessPoint(0);
    }
  }
  return pref_ap;
}
std::vector<Point3D> getAccessPoints(const frBlockObject* pin)
{
  std::vector<Point3D> result;
  if (pin->typeId() == frcInstTerm) {
    auto iterm = static_cast<const frInstTerm*>(pin);
    auto transform = iterm->getInst()->getNoRotationTransform();
    const int pin_access_idx = iterm->getInst()->getPinAccessIdx();
    for (const auto& mpin : iterm->getTerm()->getPins()) {
      if (!mpin->hasPinAccess()) {
        continue;
      }
      for (const auto& ap :
           mpin->getPinAccess(pin_access_idx)->getAccessPoints()) {
        auto ap_loc = ap->getPoint();
        transform.apply(ap_loc);
        result.emplace_back(ap_loc, ap->getLayerNum());
      }
    }
  } else {
    auto bterm = static_cast<const frBTerm*>(pin);
    for (const auto& bpin : bterm->getPins()) {
      if (!bpin->hasPinAccess()) {
        continue;
      }
      for (int i = 0; i < bpin->getNumPinAccess(); i++) {
        auto pa = bpin->getPinAccess(i);
        for (const auto& ap : pa->getAccessPoints()) {
          result.emplace_back(ap->getPoint(), ap->getLayerNum());
        }
      }
    }
  }
  return result;
}

/**
 * @brief Checks if any of the pins' accesspoints is covered by the net's guides
 *
 * @param pin The pin we are checking.
 * @param guides The list of the net guides.
 * @returns True if any access point is covered by any guide.
 */
bool isPinCoveredByGuides(const frBlockObject* pin,
                          const std::vector<frRect>& guides)
{
  for (const auto& ap_loc : getAccessPoints(pin)) {
    for (const auto& guide : guides) {
      if (guide.getLayerNum() == ap_loc.z()
          && guide.getBBox().intersects(ap_loc)) {
        return true;
      }
    }
  }
  return false;
}

/**
 * @brief Returns the best gcell location to later use for patching guides.
 *
 * The function returns the gcell location with the highest number of access
 * points.
 *
 * @return The chosen best gcell index
 */
Point3D findBestPinLocation(frDesign* design,
                            frBlockObject* pin,
                            const std::vector<frRect>& guides)
{
  std::map<Point3D, int> gcell_to_ap_count;  // map from gcell index to number
                                             // of accesspoints it holds.
  frCoord min_dist_to_guides = std::numeric_limits<frCoord>::max();
  for (const auto& ap_loc : getAccessPoints(pin)) {
    auto ap_gcell_idx = design->getTopBlock()->getGCellIdx(ap_loc);
    auto gcell_center = design->getTopBlock()->getGCellCenter(ap_gcell_idx);
    for (const auto& guide : guides) {
      auto dist = odb::manhattanDistance(guide.getBBox(), gcell_center);
      if (dist < min_dist_to_guides) {
        gcell_to_ap_count.clear();
        min_dist_to_guides = dist;
        gcell_to_ap_count[Point3D(ap_gcell_idx, ap_loc.z())]++;
      } else if (dist == min_dist_to_guides) {
        gcell_to_ap_count[Point3D(ap_gcell_idx, ap_loc.z())]++;
      }
    }
  }
  Point3D best_pin_loc_idx;
  int highest_count = 0;
  for (const auto& [gcell_idx, count] : gcell_to_ap_count) {
    if (count > highest_count) {
      best_pin_loc_idx = gcell_idx;
      highest_count = count;
    }
  }
  return best_pin_loc_idx;
}
/**
 * @brief Returns the index of the closest guide to best_pin_loc_coords.
 *
 * This function iterates over the guides, finds the guide with the
 * minimal distance to best_pin_loc_coords and returns it.
 *
 * @param best_pin_loc_coords The gcell center point of the chosen pin shape
 *
 */
int findClosestGuide(const Point3D& best_pin_loc_coords,
                     const std::vector<frRect>& guides,
                     const frCoord layer_change_penalty,
                     const RouterConfiguration* router_cfg)
{
  int closest_guide_idx = 0;
  int dist = 0;
  int min_dist = std::numeric_limits<int>::max();
  int guide_idx = 0;
  for (const auto& guide : guides) {
    dist = odb::manhattanDistance(guide.getBBox(), best_pin_loc_coords);
    dist += abs(guide.getLayerNum() - best_pin_loc_coords.z())
            * layer_change_penalty;
    if (guide.getLayerNum() < router_cfg->BOTTOM_ROUTING_LAYER) {
      dist += 1e9;
    }
    if (dist < min_dist) {
      min_dist = dist;
      closest_guide_idx = guide_idx;
    }
    guide_idx++;
  }
  return closest_guide_idx;
}
/**
 * @brief Adjusts the guide point to the coordinate of the nearest gcell
 * center.
 *
 * Given a point X on a guide perimeter, this function adjust it to X' which
 * is the center of the nearest gcell from the most left, right, north, or
 * south gcells of the guide. See the following figure for more clarification
 *
 * =========================X===========
 * |        |        |        |        |
 * |        |        |        |        |
 * |        |        |        |    X'  |
 * |        |        |        |        |
 * |        |        |        |        |
 * =====================================
 * @param guide_pt A point on the guide perimeter that is adjusted from X to
 * X'
 * @param guide_bbox The bounding box of the guide on which relies guide_pt
 * @param gcell_half_size_horz Half the horizontal size of the gcell
 * @param gcell_half_size_vert Half the vertical size of the gcell
 *
 */
void adjustGuidePoint(Point3D& guide_pt,
                      const odb::Rect& guide_bbox,
                      const frCoord gcell_half_size_horz,
                      const frCoord gcell_half_size_vert)
{
  if (std::abs(guide_bbox.xMin() - guide_pt.x())
      <= std::abs(guide_bbox.xMax() - guide_pt.x())) {
    guide_pt.setX(guide_bbox.xMin() + gcell_half_size_horz);
  } else {
    guide_pt.setX(guide_bbox.xMax() - gcell_half_size_horz);
  }
  if (std::abs(guide_bbox.yMin() - guide_pt.y())
      <= std::abs(guide_bbox.yMax() - guide_pt.y())) {
    guide_pt.setY(guide_bbox.yMin() + gcell_half_size_vert);
  } else {
    guide_pt.setY(guide_bbox.yMax() - gcell_half_size_vert);
  }
}
/**
 * @brief Extends the chosen guide to cover the best_pin_loc_coords.
 *
 * The function extends the chosen guide to cover the best_pin_loc_coords if
 * possible. The function also adjusts the guide_pt to the new extension.
 * Check the following figure where X is the original guide_pt, b_pt is the
 * best_pin_loc_coords. X' = b_pt should be the resulting guide_pt.
 *
 * =====================================---------------------
 * |                                   |                    |
 * |                                   |      Extension     |
 * |          OriginalGuide      X     |        b_pt        |
 * |                                   |                    |
 * |                                   |                    |
 * =====================================---------------------
 *
 * @param best_pin_loc_coords The gcell center point of the chosen pin shape
 * @param gcell_half_size_horz Half the horizontal size of the gcell
 * @param gcell_half_size_vert Half the vertical size of the gcell
 * @param guide The guide we are attempting to extend to cover
 * best_pin_loc_coords
 * @param guide_pt The center of the gcell on the guide from which we should
 * extend. See `adjustGuidePoint()` as this guide_pt is after adjustment. It
 * is updated if the guide is extended.
 * @see adjustGuidePoint()
 */
void extendGuide(frDesign* design,
                 const odb::Point& best_pin_loc_coords,
                 const frCoord gcell_half_size_horz,
                 const frCoord gcell_half_size_vert,
                 frRect& guide,
                 Point3D& guide_pt)
{
  const odb::Rect& guide_bbox = guide.getBBox();
  // connect best_pin_loc to guide_pt by trying to extend the closest guide
  if (design->isHorizontalLayer(guide_pt.z())) {
    if (guide_pt.x() != best_pin_loc_coords.x()) {
      if (best_pin_loc_coords.x() < guide_bbox.xMin()) {
        guide.setLeft(best_pin_loc_coords.x() - gcell_half_size_horz);
      } else if (best_pin_loc_coords.x() > guide_bbox.xMax()) {
        guide.setRight(best_pin_loc_coords.x() + gcell_half_size_horz);
      }
      guide_pt.setX(best_pin_loc_coords.x());
    }
  } else if (design->isVerticalLayer(guide_pt.z())) {
    if (guide_pt.y() != best_pin_loc_coords.y()) {
      if (best_pin_loc_coords.y() < guide_bbox.yMin()) {
        guide.setBottom(best_pin_loc_coords.y() - gcell_half_size_vert);
      } else if (best_pin_loc_coords.y() > guide_bbox.yMax()) {
        guide.setTop(best_pin_loc_coords.y() + gcell_half_size_vert);
      }
      guide_pt.setY(best_pin_loc_coords.y());
    }
  }
}
/**
 * @brief Creates guides of 1-gcell size on all layers between the current
 * guides and the pin z coordinate.
 *
 * The function creates guides of 1-gcell size centered at best_pin_loc_coords
 * on all layers in the range ]start_z, best_pin_loc_coords.z()]. start_z is
 * the layerNum of the chosen closest guide.
 *
 * @param best_pin_loc_coords The gcell center point of the chosen pin shape
 * @param start_z The layerNum of the chosen closest guide.
 * @param gcell_half_size_horz Half the horizontal size of the gcell
 * @param gcell_half_size_vert Half the vertical size of the gcell
 */
void fillGuidesUpToZ(const Point3D& best_pin_loc_coords,
                     const int start_z,
                     const frCoord gcell_half_size_horz,
                     const frCoord gcell_half_size_vert,
                     frNet* net,
                     std::vector<frRect>& guides)
{
  const int inc = start_z < best_pin_loc_coords.z() ? 2 : -2;
  for (frLayerNum curr_z = start_z + inc;
       curr_z != best_pin_loc_coords.z() + inc;
       curr_z += inc) {
    guides.emplace_back(best_pin_loc_coords.x() - gcell_half_size_horz,
                        best_pin_loc_coords.y() - gcell_half_size_vert,
                        best_pin_loc_coords.x() + gcell_half_size_horz,
                        best_pin_loc_coords.y() + gcell_half_size_vert,
                        curr_z,
                        net);
  }
}

/**
 * @brief logs the number of guides read so far
 *
 * If the number of guides read is less than 1M guides, the function reports
 * every 100k guides, otherwise it reports every 1M guides read
 * @param num_guides number of guides read so far
 */
void logGuidesRead(const int num_guides, utl::Logger* logger)
{
  bool log = false;
  if (num_guides < 1000000) {
    log = (num_guides % 100000 == 0);
  } else {
    log = (num_guides % 1000000 == 0);
  }
  if (log) {
    logger->info(DRT, 156, "guideIn read {} guides.", num_guides);
  }
}
/**
 * @brief Initializes guide intervals for genGuides_merge.
 *
 * The function iterates over all the net guides and identifies each one's
 * coverage of the gcell indices. The data structure merges touching guides as
 * they intersect in their intervals. For a guide on a horizontal layer spanning
 * from index(1, 10) to (10, 12). It adds the following entries to
 * intvs[layer_num]:
 * - Track 10 begin_idx 1 end_idx 10 : intvs[layer_num][10].insert(1, 10)
 * - Track 11 begin_idx 1 end_idx 10 : intvs[layer_num][11].insert(1, 10)
 * - Track 12 begin_idx 1 end_idx 10 : intvs[layer_num][12].insert(1, 10)
 * As you can see the map key/track idx is the vertical index because this is a
 * horizontal layer.
 * @param rects The list of guides of the net.
 * @param intvs The map of guide intervals.
 */
void initGuideIntervals(const std::vector<frRect>& rects,
                        const frDesign* design,
                        TrackIntervalsByLayer& intvs)
{
  for (const auto& rect : rects) {
    const odb::Rect box = rect.getBBox();
    const odb::Point pt1(box.ll());
    const odb::Point idx1 = design->getTopBlock()->getGCellIdx(pt1);
    const frCoord x1 = idx1.x();
    const frCoord y1 = idx1.y();
    const odb::Point pt2(box.xMax() - 1, box.yMax() - 1);
    const odb::Point idx2 = design->getTopBlock()->getGCellIdx(pt2);
    const frCoord x2 = idx2.x();
    const frCoord y2 = idx2.y();
    const frLayerNum layer_num = rect.getLayerNum();
    const bool is_horizontal = design->getTech()->getLayer(layer_num)->getDir()
                               == dbTechLayerDir::HORIZONTAL;
    if (is_horizontal) {
      for (auto track_idx = y1; track_idx <= y2; track_idx++) {
        intvs[layer_num][track_idx].insert(Interval::closed(x1, x2));
      }
    } else {
      for (auto track_idx = x1; track_idx <= x2; track_idx++) {
        intvs[layer_num][track_idx].insert(Interval::closed(y1, y2));
      }
    }
  }
}

bool gcellHasGuide(frCoord pivot_idx,
                   frCoord other_idx,
                   const frLayerNum curr_layer_num,
                   const TrackIntervalsByLayer& intvs)
{
  if (curr_layer_num % 4 == 2) {
    std::swap(pivot_idx, other_idx);
  }
  for (frLayerNum layer_num = 0; layer_num < intvs.size(); layer_num += 2) {
    const auto& layer_intvs = intvs[layer_num];
    auto it = layer_intvs.find(pivot_idx);
    if (it != layer_intvs.end()
        && boost::icl::contains(it->second, other_idx)) {
      return true;
    }
    if (curr_layer_num % 2 == 0) {
      std::swap(pivot_idx, other_idx);
    }
  }
  return false;
}
/**
 * @brief Checks if there exists an interval with the specified passed
 * arguments
 *
 * The function checks if there exists an interval on layer_num with any track
 * index from begin_idx to end_idx that contains both indices; track_idx1 and
 * track_idx2. This is used for discovering bridge guides over touching guides
 * on consecutive track indices.
 * @param begin_idx The lower bound of track indices for searching.
 * @param end_idx The upper bound of track indices for searching.
 * @returns True if it finds a bridge with the specified criteria and False
 * otherwise
 */
bool hasGuideInterval(const frCoord begin_idx,
                      const frCoord end_idx,
                      const frCoord track_idx1,
                      const frCoord track_idx2,
                      const frLayerNum layer_num,
                      const TrackIntervalsByLayer& intvs)
{
  if (layer_num < 0 || layer_num >= intvs.size()) {
    return false;
  }
  for (auto it = intvs[layer_num].lower_bound(begin_idx);
       it != intvs[layer_num].end() && it->first <= end_idx;
       it++) {
    if (boost::icl::contains(it->second, track_idx1)
        && boost::icl::contains(it->second, track_idx2)) {
      return true;
    }
  }
  return false;
}
struct BridgeGuide
{
  frCoord track_idx{-1};
  frCoord begin_idx{-1};
  frCoord end_idx{-1};
  frLayerNum layer_num{-1};
  BridgeGuide(frCoord track_idx_in = -1,
              frCoord idx1 = -1,
              frCoord idx2 = -1,
              frLayerNum layer_num_in = -1)
      : track_idx(track_idx_in),
        begin_idx(std::min(idx1, idx2)),
        end_idx(std::max(idx1, idx2)),
        layer_num(layer_num_in)
  {
  }
  frCoord getDist() const
  {
    return layer_num == -1 ? std::numeric_limits<frCoord>::max()
                           : end_idx - begin_idx;
  }
  bool operator<(const BridgeGuide& rhs) const
  {
    const auto curr_dist = getDist();
    const auto rhs_dis = rhs.getDist();
    if (curr_dist == rhs_dis) {
      return layer_num > rhs.layer_num;
    }
    return curr_dist < rhs_dis;
  }
};
/**
 * @brief Adds Bridge guides for touching guides on the same layer if needed.
 *
 * The function identifies touching guides on the same layer on consecutive
 * track indices. It adds a bridge guide on the upper or lower layer that
 * connects both guides if such one does not exist.
 * @note The function prefers bridging on the upper layer to the lower layer.
 */
void addTouchingGuidesBridges(TrackIntervalsByLayer& intvs, utl::Logger* logger)
{
  // connect corner edges
  for (int layer_num = intvs.size() - 1; layer_num >= 0; layer_num--) {
    const auto& curr_layer_intvs = intvs[layer_num];
    int prev_track_idx = -2;
    for (const auto& [curr_track_idx, curr_track_intvs] : curr_layer_intvs) {
      if (curr_track_idx == prev_track_idx + 1) {
        const auto& prev_track_intvs = curr_layer_intvs.at(prev_track_idx);

        for (auto intv1 : curr_track_intvs) {
          for (auto intv2 : prev_track_intvs) {
            if (boost::icl::intersects(intv1, intv2)) {
              continue;
            }
            if (intv1.upper() + 1 == intv2.lower()
                && !gcellHasGuide(
                    curr_track_idx, intv1.upper() + 1, layer_num, intvs)
                && !gcellHasGuide(
                    prev_track_idx, intv1.upper(), layer_num, intvs)) {
              intvs[layer_num][curr_track_idx].insert(
                  Interval::closed(intv1.upper(), intv1.upper() + 1));
            } else if (intv2.upper() + 1 == intv1.lower()
                       && !gcellHasGuide(
                           prev_track_idx, intv2.upper() + 1, layer_num, intvs)
                       && !gcellHasGuide(
                           curr_track_idx, intv2.upper(), layer_num, intvs)) {
              intvs[layer_num][prev_track_idx].insert(
                  Interval::closed(intv2.upper(), intv2.upper() + 1));
            }
          }
        }
      }
      prev_track_idx = curr_track_idx;
    }
  }

  std::vector<BridgeGuide> bridge_guides;
  // append touching edges
  for (int layer_num = 0; layer_num < (int) intvs.size(); layer_num++) {
    const auto& curr_layer_intvs = intvs[layer_num];
    int prev_track_idx = -2;
    for (const auto& [curr_track_idx, curr_track_intvs] : curr_layer_intvs) {
      if (curr_track_idx == prev_track_idx + 1) {
        const auto& prev_track_intvs = curr_layer_intvs.at(prev_track_idx);
        const auto intvs_intersection = curr_track_intvs & prev_track_intvs;
        for (const auto& intv : intvs_intersection) {
          const auto begin_idx = intv.lower();
          const auto end_idx = intv.upper();
          bool has_bridge = false;
          // lower layer intersection
          std::optional<frLayerNum> bridge_layer_num;
          if (layer_num - 2 >= 0) {
            bridge_layer_num = layer_num - 2;
            has_bridge = hasGuideInterval(begin_idx,
                                          end_idx,
                                          curr_track_idx,
                                          prev_track_idx,
                                          bridge_layer_num.value(),
                                          intvs);
          }
          if (layer_num + 2 < (int) intvs.size() && !has_bridge) {
            bridge_layer_num = layer_num + 2;
            has_bridge = hasGuideInterval(begin_idx,
                                          end_idx,
                                          curr_track_idx,
                                          prev_track_idx,
                                          bridge_layer_num.value(),
                                          intvs);
          }
          if (!has_bridge) {
            // add bridge guide;
            if (!bridge_layer_num.has_value()) {
              logger->error(
                  DRT, 228, "genGuides_merge cannot find bridge layer.");
            }
            bridge_guides.emplace_back(begin_idx,
                                       prev_track_idx,
                                       curr_track_idx,
                                       bridge_layer_num.value());
          }
        }
      }
      prev_track_idx = curr_track_idx;
    }
  }

  for (const auto& bridge : bridge_guides) {
    intvs[bridge.layer_num][bridge.track_idx].insert(
        Interval::closed(bridge.begin_idx, bridge.end_idx));
  }
}

std::vector<int> getVisitedIndices(const std::set<int>& indices,
                                   const std::vector<bool>& visited)
{
  std::vector<int> visited_indices;
  std::copy_if(indices.begin(),
               indices.end(),
               std::back_inserter(visited_indices),
               [&visited](int idx) { return visited[idx]; });
  return visited_indices;
}

}  // namespace

bool GuideProcessor::isValidGuideLayerNum(odb::dbGuide* db_guide,
                                          frNet* net,
                                          frLayerNum& layer_num)
{
  bool error = false;
  frLayer* layer = getTech()->getLayer(db_guide->getLayer()->getName());
  if (layer == nullptr) {
    logger_->error(
        DRT, 154, "Cannot find layer {}.", db_guide->getLayer()->getName());
  }
  layer_num = layer->getLayerNum();

  // Ignore guide as invalid if above top routing layer for a net with bterms
  // above top routing layer
  if (layer_num > router_cfg_->TOP_ROUTING_LAYER) {
    if (net->hasBTermsAboveTopLayer()) {
      return false;
    }
    error = true;
  }
  if (layer_num < router_cfg_->BOTTOM_ROUTING_LAYER) {
    // check if this is a via access guide
    if (!getDesign()->getTopBlock()->getGCellPatterns().empty()) {
      auto guide_rect = db_guide->getBox();
      guide_rect.bloat(-1, guide_rect);
      const bool one_gcell_guide
          = getDesign()->getTopBlock()->getGCellIdx(guide_rect.ll())
            == getDesign()->getTopBlock()->getGCellIdx(guide_rect.ur());
      if (!one_gcell_guide) {
        // TODO: uncomment this when GRT issue is solved
        // error = true;  // not a valid via access guide
      }
    }
    // else I don't know how many gcells the guide spans
  }
  if (error) {
    logger_->error(
        DRT,
        155,
        "Guide in net {} uses layer {} ({})"
        " that is outside the allowed routing range "
        "[{} ({}), {} ({})] with via access on [{} ({})].",
        net->getName(),
        layer->getName(),
        layer_num,
        getTech()->getLayer(router_cfg_->BOTTOM_ROUTING_LAYER)->getName(),
        router_cfg_->BOTTOM_ROUTING_LAYER,
        getTech()->getLayer(router_cfg_->TOP_ROUTING_LAYER)->getName(),
        router_cfg_->TOP_ROUTING_LAYER,
        getTech()->getLayer(router_cfg_->VIA_ACCESS_LAYERNUM)->getName(),
        router_cfg_->VIA_ACCESS_LAYERNUM);
  }
  return true;
}

void GuideProcessor::readGCellGrid()
{
  auto gcellGrid = db_->getChip()->getBlock()->getGCellGrid();
  if (gcellGrid == nullptr || gcellGrid->getNumGridPatternsX() != 1
      || gcellGrid->getNumGridPatternsY() != 1) {
    return;
  }
  frGCellPattern xgp, ygp;
  frCoord GCELLOFFSETX, GCELLOFFSETY, GCELLGRIDX, GCELLGRIDY;
  frCoord COUNTX, COUNTY;
  gcellGrid->getGridPatternX(0, GCELLOFFSETX, COUNTX, GCELLGRIDX);
  gcellGrid->getGridPatternY(0, GCELLOFFSETY, COUNTY, GCELLGRIDY);
  xgp.setStartCoord(GCELLOFFSETX);
  xgp.setSpacing(GCELLGRIDX);
  xgp.setCount(COUNTX);
  xgp.setHorizontal(false);

  ygp.setStartCoord(GCELLOFFSETY);
  ygp.setSpacing(GCELLGRIDY);
  ygp.setCount(COUNTY);
  ygp.setHorizontal(true);
  getDesign()->getTopBlock()->setGCellPatterns(
      {std::move(xgp), std::move(ygp)});
}

bool GuideProcessor::readGuides()
{
  ProfileTask profile("IO:readGuide");

  // Read GCell grid information first
  readGCellGrid();

  int num_guides = 0;
  const auto block = db_->getChip()->getBlock();
  for (const auto db_net : block->getNets()) {
    frNet* net = getDesign()->getTopBlock()->findNet(db_net->getName());
    if (net == nullptr) {
      logger_->error(DRT, 153, "Cannot find net {}.", db_net->getName());
    }
    for (auto db_guide : db_net->getGuides()) {
      if (db_guide->isCongested()) {
        logger_->error(DRT, 352, "Input route guides are congested.");
      }
      frLayerNum layer_num;
      if (!isValidGuideLayerNum(db_guide, net, layer_num)) {
        continue;
      }
      frRect rect;
      rect.setBBox(db_guide->getBox());
      rect.setLayerNum(layer_num);
      tmp_guides_[net].emplace_back(rect);
      ++num_guides;
      logGuidesRead(num_guides, logger_);
    }
  }
  if (router_cfg_->VERBOSE > 0) {
    logger_->report("");
    logger_->info(utl::DRT, 157, "Number of guides:     {}", num_guides);
    logger_->report("");
  }
  return !tmp_guides_.empty();
}

void GuideProcessor::buildGCellPatterns_helper(frCoord& GCELLGRIDX,
                                               frCoord& GCELLGRIDY,
                                               frCoord& GCELLOFFSETX,
                                               frCoord& GCELLOFFSETY)
{
  buildGCellPatterns_getWidth(GCELLGRIDX, GCELLGRIDY);
  buildGCellPatterns_getOffset(
      GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);
}

void GuideProcessor::buildGCellPatterns_getWidth(frCoord& GCELLGRIDX,
                                                 frCoord& GCELLGRIDY)
{
  std::map<frCoord, int> guideGridXMap, guideGridYMap;
  // get GCell size information loop
  for (auto& [netName, rects] : tmp_guides_) {
    for (auto& rect : rects) {
      frLayerNum layerNum = rect.getLayerNum();
      odb::Rect guideBBox = rect.getBBox();
      frCoord guideWidth = (getTech()->getLayer(layerNum)->getDir()
                            == dbTechLayerDir::HORIZONTAL)
                               ? guideBBox.dy()
                               : guideBBox.dx();
      if (getTech()->getLayer(layerNum)->getDir()
          == dbTechLayerDir::HORIZONTAL) {
        if (guideGridYMap.find(guideWidth) == guideGridYMap.end()) {
          guideGridYMap[guideWidth] = 0;
        }
        guideGridYMap[guideWidth]++;
      } else if (getTech()->getLayer(layerNum)->getDir()
                 == dbTechLayerDir::VERTICAL) {
        if (guideGridXMap.find(guideWidth) == guideGridXMap.end()) {
          guideGridXMap[guideWidth] = 0;
        }
        guideGridXMap[guideWidth]++;
      }
    }
  }
  frCoord tmpGCELLGRIDX = -1, tmpGCELLGRIDY = -1;
  int tmpGCELLGRIDXCnt = -1, tmpGCELLGRIDYCnt = -1;
  for (const auto [coord, cnt] : guideGridXMap) {
    if (cnt > tmpGCELLGRIDXCnt) {
      tmpGCELLGRIDXCnt = cnt;
      tmpGCELLGRIDX = coord;
    }
  }
  for (const auto [coord, cnt] : guideGridYMap) {
    if (cnt > tmpGCELLGRIDYCnt) {
      tmpGCELLGRIDYCnt = cnt;
      tmpGCELLGRIDY = coord;
    }
  }
  if (tmpGCELLGRIDX != -1) {
    GCELLGRIDX = tmpGCELLGRIDX;
  } else {
    logger_->error(DRT, 170, "No GCELLGRIDX.");
  }
  if (tmpGCELLGRIDY != -1) {
    GCELLGRIDY = tmpGCELLGRIDY;
  } else {
    logger_->error(DRT, 171, "No GCELLGRIDY.");
  }
}

void GuideProcessor::buildGCellPatterns_getOffset(frCoord GCELLGRIDX,
                                                  frCoord GCELLGRIDY,
                                                  frCoord& GCELLOFFSETX,
                                                  frCoord& GCELLOFFSETY)
{
  std::map<frCoord, int> guideOffsetXMap, guideOffsetYMap;
  // get GCell offset information loop
  for (auto& [netName, rects] : tmp_guides_) {
    for (auto& rect : rects) {
      // frLayerNum layerNum = rect.getLayerNum();
      odb::Rect guideBBox = rect.getBBox();
      frCoord guideXOffset = guideBBox.xMin() % GCELLGRIDX;
      frCoord guideYOffset = guideBBox.yMin() % GCELLGRIDY;
      if (guideXOffset < 0) {
        guideXOffset = GCELLGRIDX - guideXOffset;
      }
      if (guideYOffset < 0) {
        guideYOffset = GCELLGRIDY - guideYOffset;
      }
      if (guideOffsetXMap.find(guideXOffset) == guideOffsetXMap.end()) {
        guideOffsetXMap[guideXOffset] = 0;
      }
      guideOffsetXMap[guideXOffset]++;
      if (guideOffsetYMap.find(guideYOffset) == guideOffsetYMap.end()) {
        guideOffsetYMap[guideYOffset] = 0;
      }
      guideOffsetYMap[guideYOffset]++;
    }
  }
  frCoord tmpGCELLOFFSETX = -1, tmpGCELLOFFSETY = -1;
  int tmpGCELLOFFSETXCnt = -1, tmpGCELLOFFSETYCnt = -1;
  for (const auto [coord, cnt] : guideOffsetXMap) {
    if (cnt > tmpGCELLOFFSETXCnt) {
      tmpGCELLOFFSETXCnt = cnt;
      tmpGCELLOFFSETX = coord;
    }
  }
  for (const auto [coord, cnt] : guideOffsetYMap) {
    if (cnt > tmpGCELLOFFSETYCnt) {
      tmpGCELLOFFSETYCnt = cnt;
      tmpGCELLOFFSETY = coord;
    }
  }
  if (tmpGCELLOFFSETX != -1) {
    GCELLOFFSETX = tmpGCELLOFFSETX;
  } else {
    logger_->error(DRT, 172, "No GCELLGRIDX.");
  }
  if (tmpGCELLOFFSETY != -1) {
    GCELLOFFSETY = tmpGCELLOFFSETY;
  } else {
    logger_->error(DRT, 173, "No GCELLGRIDY.");
  }
}

void GuideProcessor::buildGCellPatterns()
{
  // horizontal = false is gcell lines along y direction (x-grid)
  if (getDesign()->getTopBlock()->getGCellPatterns().empty()) {
    frGCellPattern xgp, ygp;
    frCoord GCELLOFFSETX, GCELLOFFSETY, GCELLGRIDX, GCELLGRIDY;
    odb::Rect dieBox = getDesign()->getTopBlock()->getDieBox();
    buildGCellPatterns_helper(
        GCELLGRIDX, GCELLGRIDY, GCELLOFFSETX, GCELLOFFSETY);
    xgp.setHorizontal(false);
    // find first coord >= dieBox.xMin()
    frCoord startCoordX
        = dieBox.xMin() / GCELLGRIDX * GCELLGRIDX + GCELLOFFSETX;
    if (startCoordX > dieBox.xMin()) {
      startCoordX -= GCELLGRIDX;
    }
    xgp.setStartCoord(startCoordX);
    xgp.setSpacing(GCELLGRIDX);
    if ((dieBox.xMax() - GCELLOFFSETX) / GCELLGRIDX < 1) {
      logger_->error(DRT, 174, "GCell cnt x < 1.");
    }
    xgp.setCount((dieBox.xMax() - startCoordX) / GCELLGRIDX);

    ygp.setHorizontal(true);
    // find first coord >= dieBox.yMin()
    frCoord startCoordY
        = dieBox.yMin() / GCELLGRIDY * GCELLGRIDY + GCELLOFFSETY;
    if (startCoordY > dieBox.yMin()) {
      startCoordY -= GCELLGRIDY;
    }
    ygp.setStartCoord(startCoordY);
    ygp.setSpacing(GCELLGRIDY);
    if ((dieBox.yMax() - GCELLOFFSETY) / GCELLGRIDY < 1) {
      logger_->error(DRT, 175, "GCell cnt y < 1.");
    }
    ygp.setCount((dieBox.yMax() - startCoordY) / GCELLGRIDY);
    getDesign()->getTopBlock()->setGCellPatterns(
        {std::move(xgp), std::move(ygp)});
  }
  const auto& gcell_patterns = getDesign()->getTopBlock()->getGCellPatterns();
  const auto& xgp = gcell_patterns[0];
  const auto& ygp = gcell_patterns[1];
  if (router_cfg_->VERBOSE > 0 || logger_->debugCheck(DRT, "autotuner", 1)) {
    logger_->info(DRT,
                  176,
                  "GCELLGRID X {} DO {} STEP {} ;",
                  xgp.getStartCoord(),
                  xgp.getCount(),
                  xgp.getSpacing());
    logger_->info(DRT,
                  177,
                  "GCELLGRID Y {} DO {} STEP {} ;",
                  ygp.getStartCoord(),
                  ygp.getCount(),
                  ygp.getSpacing());
  }
}

void GuideProcessor::connectGuidesWithBestPinLoc(
    Point3D& guide_pt,
    const odb::Point& best_pin_loc_coords,
    const frCoord gcell_half_size_horz,
    const frCoord gcell_half_size_vert,
    frNet* net,
    std::vector<frRect>& guides)
{
  if (guide_pt.x() != best_pin_loc_coords.x()
      || guide_pt.y() != best_pin_loc_coords.y()) {
    const odb::Point pl = {std::min(best_pin_loc_coords.x(), guide_pt.x()),
                           std::min(best_pin_loc_coords.y(), guide_pt.y())};
    const odb::Point ph = {std::max(best_pin_loc_coords.x(), guide_pt.x()),
                           std::max(best_pin_loc_coords.y(), guide_pt.y())};
    bool is_horizontal = pl.x() != ph.x();
    bool is_vertical = pl.y() != ph.y();
    frLayerNum layer_num = guide_pt.z();
    if (is_horizontal ^ is_vertical) {
      if ((is_vertical && design_->isHorizontalLayer(layer_num))
          || (is_horizontal && design_->isVerticalLayer(layer_num))) {
        if (layer_num + 2 <= router_cfg_->TOP_ROUTING_LAYER) {
          layer_num += 2;
        } else {
          layer_num -= 2;
        }
      }
    }
    guide_pt.setZ(layer_num);
    guides.emplace_back(pl.x() - gcell_half_size_horz,
                        pl.y() - gcell_half_size_vert,
                        ph.x() + gcell_half_size_horz,
                        ph.y() + gcell_half_size_vert,
                        layer_num,
                        net);
  }
}

void GuideProcessor::patchGuides_helper(frNet* net,
                                        std::vector<frRect>& guides,
                                        const Point3D& best_pin_loc_idx,
                                        const Point3D& best_pin_loc_coords,
                                        const int closest_guide_idx)
{
  Point3D guide_pt(
      getClosestPoint(guides[closest_guide_idx], best_pin_loc_coords),
      guides[closest_guide_idx].getLayerNum());
  const odb::Rect& guide_bbox = guides[closest_guide_idx].getBBox();
  const frCoord gcell_half_size_horz
      = getDesign()->getTopBlock()->getGCellSizeHorizontal() / 2;
  const frCoord gcell_half_size_vert
      = getDesign()->getTopBlock()->getGCellSizeVertical() / 2;
  adjustGuidePoint(
      guide_pt, guide_bbox, gcell_half_size_horz, gcell_half_size_vert);
  extendGuide(getDesign(),
              best_pin_loc_coords,
              gcell_half_size_horz,
              gcell_half_size_vert,
              guides[closest_guide_idx],
              guide_pt);
  if (guide_pt == best_pin_loc_coords) {
    return;
  }
  connectGuidesWithBestPinLoc(guide_pt,
                              best_pin_loc_coords,
                              gcell_half_size_horz,
                              gcell_half_size_vert,
                              net,
                              guides);
  // fill the gap between current layer and the best_pin_loc_coords layer with
  // guides
  fillGuidesUpToZ(best_pin_loc_coords,
                  guide_pt.z(),
                  gcell_half_size_horz,
                  gcell_half_size_vert,
                  net,
                  guides);
}

void GuideProcessor::patchGuides(frNet* net,
                                 frBlockObject* pin,
                                 std::vector<frRect>& guides)
{
  if (pin->typeId() != frcBTerm && pin->typeId() != frcInstTerm) {
    logger_->error(DRT, 1007, "patchGuides invoked with non-term object.");
  }
  if (isPinCoveredByGuides(pin, guides)) {
    return;
  }
  // no guide was found that overlaps with any of the pin shapes, then we patch
  // the guides

  const Point3D best_pin_loc_idx
      = findBestPinLocation(getDesign(), pin, guides);
  // The x/y/z coordinates of best_pin_loc_idx
  const Point3D best_pin_loc_coords(
      getDesign()->getTopBlock()->getGCellCenter(best_pin_loc_idx),
      best_pin_loc_idx.z());
  // get the guide that is closest to the gCell
  // TODO: test passing layer_change_penalty = gcell size
  const int closest_guide_idx
      = findClosestGuide(best_pin_loc_coords, guides, 1, router_cfg_);

  patchGuides_helper(
      net, guides, best_pin_loc_idx, best_pin_loc_coords, closest_guide_idx);
}

void GuideProcessor::coverPins(frNet* net, std::vector<frRect>& guides)
{
  for (auto pin : net->getInstTerms()) {
    patchGuides(net, pin, guides);
  }
  for (auto pin : net->getBTerms()) {
    patchGuides(net, pin, guides);
  }
}

void GuideProcessor::genGuides_prep(const std::vector<frRect>& rects,
                                    TrackIntervalsByLayer& intvs)
{
  initGuideIntervals(rects, getDesign(), intvs);
  addTouchingGuidesBridges(intvs, logger_);
}

namespace split {
/**
 * Split the range[begin_idx, end_idx] by the locations of pins in that range.
 *
 * The function searches for indices on layer_num in the range [begin_idx,
 * end_idx] where there exists pins. It adds tose indices to the split_indices
 * set. It also fills the pin_gcell_map to map pin objects to their
 * corresponding gcells.
 *
 * @param pin_helper a 3-d map where the first key is the layer_num, second key
 * is the track index orthogonal to the routing direction, and third key is the
 * track index along the routing direction. The value is the set of pins that
 * exists at the mapped location.
 * @param is_horizontal True if the layer of layer_num is horizontal
 * @param pin_gcell_map Map from pin object to set of gcell indices where the
 * pin touches.
 * @param split_indices The returned set of indices where there exists pins.
 */
void splitByPins(
    const std::vector<std::map<frCoord, std::map<frCoord, frBlockObjectSet>>>&
        pin_helper,
    const frLayerNum layer_num,
    const bool is_horizontal,
    const frCoord track_idx,
    const frCoord begin_idx,
    const frCoord end_idx,
    frBlockObjectMap<std::set<Point3D>>& pin_gcell_map,
    std::set<frCoord>& split_indices)
{
  const auto& layer_pins = pin_helper.at(layer_num);
  const auto& track_it = layer_pins.find(track_idx);
  if (track_it != layer_pins.end()) {
    const auto& pins_locations_map
        = (*track_it).second;  // track indices along the routing dir
    auto it = pins_locations_map.lower_bound(begin_idx);
    while (it != pins_locations_map.end()) {
      const auto along_routing_dir_idx = it->first;
      const auto& pins = it->second;
      if (along_routing_dir_idx > end_idx) {
        break;
      }
      split_indices.insert(along_routing_dir_idx);
      for (const auto pin : pins) {
        if (is_horizontal) {
          pin_gcell_map[pin].insert(
              Point3D(along_routing_dir_idx, track_idx, layer_num));
        } else {
          pin_gcell_map[pin].insert(
              Point3D(track_idx, along_routing_dir_idx, layer_num));
        }
      }
      ++it;
    }
  }
}
/**
 * Finds split indices where tracks on a given layer intersect with a specified
 * track.
 *
 * This function checks for track intervals on the specified `layer_num` that
 * intersect with the provided track (which can be either horizontal or
 * vertical) on the current layer. The function searches for intervals on tracks
 * within the range [begin_idx, end_idx] for the track's coordinates. If
 * intersections are found, the indices of those intersecting tracks are added
 * to the `split_indices` set.
 *
 * @param layer_num The neighbor layer of the current track's layer.
 * @param intvs Map of track intervals for each layer.
 * @param track_idx The index of the track on the current layer to check for
 * intersections.
 * @param begin_idx The beginning coordinate of the track's range.
 * @param end_idx The ending coordinate of the track's range.
 * @param split_indices A set to store the indices where intersections are
 * found.
 */
void splitByLayerIntersection(const frLayerNum layer_num,
                              const TrackIntervalsByLayer& intvs,
                              const frCoord track_idx,
                              const frCoord begin_idx,
                              const frCoord end_idx,
                              std::set<frCoord>& split_indices)
{
  if (layer_num < 0 || layer_num >= intvs.size()) {
    return;
  }
  auto it = intvs[layer_num].lower_bound(begin_idx);
  while (it != intvs[layer_num].end()) {
    const auto curr_track_idx = it->first;
    const auto& indices = it->second;
    if (curr_track_idx > end_idx) {
      break;
    }
    if (boost::icl::contains(indices, track_idx)) {
      split_indices.insert(curr_track_idx);
    }
    it++;
  }
}

/**
 * @brief Adds a rectangle to the list of rects.
 *
 * This function creates a guide rectangle with the specified gcell indices and
 * add it to the list of rects.
 *
 * @param track_idx The index of the track where the rectangle will be
 * created.(y index if the current layer is vertical and x if otherwise)
 * @param begin_idx The beginning coordinate for the rectangle.
 * @param end_idx The ending coordinate for the rectangle.
 * @param layer_num The layer number the rectangle belongs to.
 * @param is_horizontal True if the current layer is horizontal.
 * @param rects vector of guides(defined gcell indices).
 *
 * @return void
 */
void addSplitRect(const frCoord track_idx,
                  const frCoord begin_idx,
                  const frCoord end_idx,
                  const frLayerNum layer_num,
                  const bool is_horizontal,
                  std::vector<frRect>& rects)
{
  frRect rect;
  if (is_horizontal) {
    rect.setBBox(odb::Rect(begin_idx, track_idx, end_idx, track_idx));
  } else {
    rect.setBBox(odb::Rect(track_idx, begin_idx, track_idx, end_idx));
  }
  rect.setLayerNum(layer_num);
  rects.emplace_back(rect);
}
}  // namespace split

void GuideProcessor::genGuides_split(
    std::vector<frRect>& rects,
    const TrackIntervalsByLayer& intvs,
    const std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
    frBlockObjectMap<std::set<Point3D>>& pin_gcell_map,
    bool via_access_only) const
{
  rects.clear();
  // layer_num->track_idx->routing_dir_track_idx->set of pins
  std::vector<std::map<frCoord, std::map<frCoord, frBlockObjectSet>>>
      pin_helper(getTech()->getLayers().size());
  for (const auto& [point, pins] : gcell_pin_map) {
    if (getTech()->getLayer(point.z())->getDir()
        == dbTechLayerDir::HORIZONTAL) {
      pin_helper[point.z()][point.y()][point.x()] = pins;
    } else {
      pin_helper[point.z()][point.x()][point.y()] = pins;
    }
  }

  for (int layer_num = 0; layer_num < (int) intvs.size(); layer_num++) {
    auto dir = getTech()->getLayer(layer_num)->getDir();
    const bool is_horizontal = dir == dbTechLayerDir::HORIZONTAL;
    for (auto& [track_idx, curr_intvs] : intvs[layer_num]) {
      // split by lower/upper seg
      for (const auto& intv : curr_intvs) {
        auto begin_idx = intv.lower();
        auto end_idx = intv.upper();
        std::set<frCoord> split_indices;
        // hardcode layerNum <= VIA_ACCESS_LAYERNUM not used for GR
        if (via_access_only && layer_num <= router_cfg_->VIA_ACCESS_LAYERNUM) {
          // split by pin
          split::splitByPins(pin_helper,
                             layer_num,
                             is_horizontal,
                             track_idx,
                             begin_idx,
                             end_idx,
                             pin_gcell_map,
                             split_indices);
          // for first iteration, consider guides below or
          // on VIA_ACCESS_LAYER_NUM as via guides only.
          for (int curr_idx = begin_idx; curr_idx <= end_idx; curr_idx++) {
            split::addSplitRect(
                track_idx, curr_idx, curr_idx, layer_num, is_horizontal, rects);
          }
        } else {
          // lower layer intersection
          split::splitByLayerIntersection(layer_num - 2,
                                          intvs,
                                          track_idx,
                                          begin_idx,
                                          end_idx,
                                          split_indices);
          // upper layer intersection
          split::splitByLayerIntersection(layer_num + 2,
                                          intvs,
                                          track_idx,
                                          begin_idx,
                                          end_idx,
                                          split_indices);
          // split by pin
          split::splitByPins(pin_helper,
                             layer_num,
                             is_horizontal,
                             track_idx,
                             begin_idx,
                             end_idx,
                             pin_gcell_map,
                             split_indices);
          // add rect
          if (split_indices.empty()) {
            logger_->error(DRT,
                           229,
                           "genGuides_split split_indices is empty on {}.",
                           getTech()->getLayer(layer_num)->getName());
          }
          if (split_indices.size() == 1) {
            auto split_idx = *(split_indices.begin());
            split::addSplitRect(track_idx,
                                split_idx,
                                split_idx,
                                layer_num,
                                is_horizontal,
                                rects);
          } else {
            auto curr_idx_it = split_indices.begin();
            split::addSplitRect(track_idx,
                                *curr_idx_it,
                                *curr_idx_it,
                                layer_num,
                                is_horizontal,
                                rects);
            auto prev_idx_it = curr_idx_it++;
            while (curr_idx_it != split_indices.end()) {
              split::addSplitRect(track_idx,
                                  *curr_idx_it,
                                  *curr_idx_it,
                                  layer_num,
                                  is_horizontal,
                                  rects);
              split::addSplitRect(track_idx,
                                  *prev_idx_it,
                                  *curr_idx_it,
                                  layer_num,
                                  is_horizontal,
                                  rects);
              prev_idx_it = curr_idx_it++;
            }
          }
        }
      }
    }
  }
  rects.shrink_to_fit();
}

void GuideProcessor::mapPinShapesToGCells(
    std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
    frBlockObject* term) const
{
  const auto pin_shapes = getPinShapes(term);
  for (const auto& shape : pin_shapes) {
    const auto layer_num = shape.getLayerNum();
    const odb::Rect box = shape.getBBox();
    const odb::Point min_idx = getDesign()->getTopBlock()->getGCellIdx(
        {box.xMin() + 1, box.yMin() + 1});
    const odb::Point max_idx = getDesign()->getTopBlock()->getGCellIdx(
        {box.xMax() - 1, box.yMax() - 1});
    for (int x = min_idx.x(); x <= max_idx.x(); x++) {
      for (int y = min_idx.y(); y <= max_idx.y(); y++) {
        gcell_pin_map[Point3D(x, y, layer_num)].insert(term);
      }
    }
  }
}

void GuideProcessor::initGCellPinMap(
    const frNet* net,
    std::map<Point3D, frBlockObjectSet>& gcell_pin_map) const
{
  for (auto instTerm : net->getInstTerms()) {
    mapTermAccessPointsToGCells(gcell_pin_map, instTerm);
  }
  for (auto term : net->getBTerms()) {
    mapTermAccessPointsToGCells(gcell_pin_map, term);
  }
}

void GuideProcessor::mapTermAccessPointsToGCells(
    std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
    frBlockObject* pin) const
{
  for (const auto& ap_loc : getAccessPoints(pin)) {
    for (const auto& idx :
         getDesign()->getTopBlock()->getGCellIndices(ap_loc)) {
      gcell_pin_map[Point3D(idx, ap_loc.z())].insert(pin);
    }
  }
}

void GuideProcessor::initPinGCellMap(
    frNet* net,
    frBlockObjectMap<std::set<Point3D>>& pin_gcell_map)
{
  for (auto& instTerm : net->getInstTerms()) {
    pin_gcell_map[instTerm];
  }
  for (auto& term : net->getBTerms()) {
    pin_gcell_map[term];
  }
}

void GuideProcessor::genGuides_addCoverGuide(frNet* net,
                                             std::vector<frRect>& rects)
{
  for (auto& instTerm : net->getInstTerms()) {
    genGuides_addCoverGuide_helper(instTerm, rects);
  }
}

void GuideProcessor::genGuides_addCoverGuide_helper(frInstTerm* iterm,
                                                    std::vector<frRect>& rects)
{
  const frInst* inst = iterm->getInst();
  const size_t num_pins = iterm->getTerm()->getPins().size();
  odb::dbTransform transform = inst->getNoRotationTransform();
  for (int pin_idx = 0; pin_idx < num_pins; pin_idx++) {
    const frAccessPoint* pref_ap = getPrefAp(iterm, pin_idx);
    if (pref_ap) {
      odb::Point pt = pref_ap->getPoint();
      transform.apply(pt);
      const odb::Point idx = getDesign()->getTopBlock()->getGCellIdx(pt);
      const odb::Rect ll_box = getDesign()->getTopBlock()->getGCellBox(
          odb::Point(idx.x() - 1, idx.y() - 1));
      const odb::Rect ur_box = getDesign()->getTopBlock()->getGCellBox(
          odb::Point(idx.x() + 1, idx.y() + 1));
      const odb::Rect cover_box(
          ll_box.xMin(), ll_box.yMin(), ur_box.xMax(), ur_box.yMax());
      const frLayerNum begin_layer_num = pref_ap->getLayerNum();
      const frLayerNum end_layer_num
          = std::min(begin_layer_num + 4, getTech()->getTopLayerNum());

      for (auto layer_num = begin_layer_num; layer_num <= end_layer_num;
           layer_num += 2) {
        frRect cover_guide;
        cover_guide.setBBox(cover_box);
        cover_guide.setLayerNum(layer_num);
        rects.push_back(cover_guide);
      }
    }
  }
}

std::vector<std::pair<frBlockObject*, odb::Point>> GuideProcessor::genGuides(
    frNet* net,
    std::vector<frRect> rects)
{
  net->clearGuides();

  coverPins(net, rects);

  int size = (int) getTech()->getLayers().size();
  if (router_cfg_->TOP_ROUTING_LAYER < std::numeric_limits<int>::max()
      && router_cfg_->TOP_ROUTING_LAYER >= 0) {
    size = std::min(size, router_cfg_->TOP_ROUTING_LAYER + 1);
  }
  TrackIntervalsByLayer intvs(size);
  if (router_cfg_->DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    genGuides_addCoverGuide(net, rects);
  }
  genGuides_prep(rects, intvs);

  std::map<Point3D, frBlockObjectSet> gcell_pin_map;
  frBlockObjectMap<std::set<Point3D>> pin_gcell_map;
  initGCellPinMap(net, gcell_pin_map);
  initPinGCellMap(net, pin_gcell_map);

  // Run for 3 iterations max
  for (int i = 0; i < 4; i++) {
    const bool force_pin_feed_through = (i >= 2);
    const bool via_access_only = (i == 0);
    const bool patch_guides_on_failure = (i == 2);
    if (i != 2)  // no change in rects in this iteration
    {
      genGuides_split(
          rects,
          intvs,
          gcell_pin_map,
          pin_gcell_map,
          via_access_only);  // split on LU intersecting guides and pins
      if (pin_gcell_map.empty()) {
        logger_->warn(DRT, 214, "genGuides empty gcell_pin_map.");
        debugPrint(logger_,
                   DRT,
                   "io",
                   1,
                   "gcell2pin.size() = {}",
                   gcell_pin_map.size());
      }
      for (auto& [obj, indices] : pin_gcell_map) {
        if (indices.empty()) {
          switch (obj->typeId()) {
            case frcInstTerm: {
              auto ptr = static_cast<frInstTerm*>(obj);
              logger_->warn(DRT,
                            215,
                            "Pin {}/{} not covered by guide.",
                            ptr->getInst()->getName(),
                            ptr->getTerm()->getName());
              break;
            }
            case frcBTerm: {
              auto ptr = static_cast<frBTerm*>(obj);
              logger_->warn(
                  DRT, 216, "Pin PIN/{} not covered by guide.", ptr->getName());
              break;
            }
            default: {
              logger_->warn(DRT, 217, "genGuides unknown type.");
              break;
            }
          }
        }
      }
    }
    GuidePathFinder path_finder(design_,
                                logger_,
                                router_cfg_,
                                net,
                                force_pin_feed_through,
                                rects,
                                pin_gcell_map);
    path_finder.setAllowWarnings(i != 0);
    if (path_finder.traverseGraph()) {
      return path_finder.commitPathToGuides(rects, pin_gcell_map);
    }
    if (patch_guides_on_failure) {
      path_finder.connectDisconnectedComponents(rects, intvs);
    }
  }
  logger_->error(
      DRT, 218, "Guide is not connected to design for net {}", net->getName());
  return {};
}

GuidePathFinder::GuidePathFinder(
    frDesign* design,
    utl::Logger* logger,
    RouterConfiguration* router_cfg,
    frNet* net,
    const bool force_feed_through,
    const std::vector<frRect>& rects,
    const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map)
    : design_(design),
      logger_(logger),
      router_cfg_(router_cfg),
      net_(net),
      force_feed_through_(force_feed_through),
      pin_gcell_map_(pin_gcell_map),
      rects_(rects)
{
  buildNodeMap(rects, pin_gcell_map);
  constructAdjList();
  is_on_path_.resize(getNodeCount(), false);
  visited_.resize(getNodeCount(), false);
  prev_idx_.resize(getNodeCount(), -1);
}

void GuidePathFinder::buildNodeMap(
    const std::vector<frRect>& rects,
    const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map)
{
  node_map_.clear();
  for (int i = 0; i < (int) rects.size(); i++) {
    const auto& rect = rects.at(i);
    odb::Rect box = rect.getBBox();
    node_map_[{box.ll(), rect.getLayerNum()}].insert(i);
    node_map_[{box.ur(), rect.getLayerNum()}].insert(i);
  }
  guide_count_ = rects.size();  // total guide cnt
  int node_idx = rects.size();
  for (const auto& [obj, gcells] : pin_gcell_map) {
    for (const auto& gcell : gcells) {
      node_map_[gcell].insert(node_idx);
    }
    ++node_idx;
  }
  node_count_ = node_idx;  // total node cnt
}

std::vector<std::vector<Point3D>> GuidePathFinder::getPinToGCellList(
    const std::vector<frRect>& rects,
    const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map,
    const std::vector<frBlockObject*>& pins) const
{
  std::vector<std::vector<Point3D>> pin_to_gcell(getPinCount());
  for (int i = 0; i < getNodeCount(); i++) {
    if (!visited_[i]) {
      continue;
    }
    int pin_idx, guide_idx;
    if (isPinIdx(i) && isGuideIdx(prev_idx_[i])) {
      pin_idx = i;
      guide_idx = prev_idx_[i];
    } else if (isGuideIdx(i) && isPinIdx(prev_idx_[i])) {
      guide_idx = i;
      pin_idx = prev_idx_[i];
    } else {
      continue;
    }
    const int true_pin_idx = getTruePinIdx(pin_idx);
    const auto& rect = rects[guide_idx];
    const odb::Rect box = rect.getBBox();
    const auto layer_num = rect.getLayerNum();
    auto pin = pins[true_pin_idx];
    if (pin_gcell_map.at(pin).find(Point3D(box.ll(), layer_num))
        != pin_gcell_map.at(pin).end()) {
      pin_to_gcell[true_pin_idx].emplace_back(box.ll(), layer_num);
    } else if (pin_gcell_map.at(pin).find(Point3D(box.ur(), layer_num))
               != pin_gcell_map.at(pin).end()) {
      pin_to_gcell[true_pin_idx].emplace_back(box.ur(), layer_num);
    } else {
      logger_->warn(
          DRT, 220, "genGuides_final net {} error 1.", net_->getName());
    }
  }
  return pin_to_gcell;
}

std::vector<std::pair<frBlockObject*, odb::Point>> GuidePathFinder::getGRPins(
    const std::vector<frBlockObject*>& pins,
    const std::vector<std::vector<Point3D>>& pin_to_gcell) const
{
  std::vector<std::pair<frBlockObject*, odb::Point>> gr_pins;
  for (int i = 0; i < getPinCount(); i++) {
    auto pin = pins[i];
    for (auto& pt : pin_to_gcell[i]) {
      odb::Point abs_pt = getDesign()->getTopBlock()->getGCellCenter(pt);
      gr_pins.emplace_back(pin, abs_pt);
    }
  }
  return gr_pins;
}

void GuidePathFinder::updateNodeMap(
    const std::vector<frRect>& rects,
    const std::vector<std::vector<Point3D>>& pin_to_gcell)
{
  node_map_.clear();
  // pin_to_gcell tells pin residency in gcell
  for (int i = 0; i < getPinCount(); i++) {
    for (auto& pt : pin_to_gcell[i]) {
      node_map_[pt].insert(i + getGuideCount());
    }
  }
  for (int i = 0; i < getGuideCount(); i++) {
    if (!visited_[i]) {
      continue;
    }
    const auto& rect = rects[i];
    odb::Rect box = rect.getBBox();
    node_map_[Point3D(box.ll(), rect.getLayerNum())].insert(i);
    node_map_[Point3D(box.ur(), rect.getLayerNum())].insert(i);
  }
}

void GuidePathFinder::clipGuides(std::vector<frRect>& rects)
{
  for (auto& [pt, indices] : node_map_) {
    const auto num_indices = indices.size();
    if (num_indices != 1) {
      continue;
    }
    const auto idx = *(indices.begin());
    if (isPinIdx(idx)) {
      logger_->error(DRT,
                     223,
                     "Pin dangling id {} ({},{}) {}.",
                     idx,
                     pt.x(),
                     pt.y(),
                     pt.z());
    }
    // no upper/lower guide
    if (node_map_.find(Point3D(pt, pt.z() + 2)) == node_map_.end()
        && node_map_.find(Point3D(pt, pt.z() - 2)) == node_map_.end()) {
      auto& rect = rects[idx];
      odb::Rect box = rect.getBBox();
      if (box.ll() == box.ur()) {
        continue;
      }
      if (box.ll() == pt) {
        rect.setBBox(odb::Rect(box.xMax(), box.yMax(), box.xMax(), box.yMax()));
      } else {
        rect.setBBox(odb::Rect(box.xMin(), box.yMin(), box.xMin(), box.yMin()));
      }
      node_map_[pt].erase(node_map_[pt].find(idx));
    }
  }
}

void GuidePathFinder::mergeGuides(std::vector<frRect>& rects)
{
  auto hasVisitedIndices = [this](const Point3D& pt) {
    if (node_map_.find(pt) == node_map_.end()) {
      return false;
    }
    const auto& indices = getVisitedIndices(node_map_.at(pt), visited_);
    return !indices.empty();
  };
  for (auto& [pt, indices] : node_map_) {
    std::vector<int> visited_indices = getVisitedIndices(indices, visited_);
    const auto num_indices = visited_indices.size();
    if (num_indices == 2) {
      const auto first_idx = *(visited_indices.begin());
      const auto second_idx = *std::prev(visited_indices.end());
      if (!isGuideIdx(first_idx) || !isGuideIdx(second_idx)) {
        continue;
      }
      // Check if there is a connection to upper or lower layer
      if (hasVisitedIndices(Point3D(pt, pt.z() + 2))
          || hasVisitedIndices(Point3D(pt, pt.z() - 2))) {
        continue;
      }
      auto& rect1 = rects[first_idx];
      auto& rect2 = rects[second_idx];
      odb::Rect box1 = rect1.getBBox();
      odb::Rect box2 = rect2.getBBox();
      if (box1.getDir() == box2.getDir()) {
        // merge both and remove rect1/box1/first_idx
        box2.merge(box1);
        rect2.setBBox(box2);
        node_map_[pt].clear();
        Point3D to_be_updated_pos;
        if (box1.ll() == pt) {
          to_be_updated_pos = Point3D(box1.ur(), pt.z());
        } else {
          to_be_updated_pos = Point3D(box1.ll(), pt.z());
        }
        auto it = node_map_[to_be_updated_pos].find(first_idx);
        node_map_[to_be_updated_pos].erase(it);
        node_map_[to_be_updated_pos].insert(second_idx);
        visited_[first_idx] = false;
      }
    }
  }
}

std::vector<std::pair<frBlockObject*, odb::Point>>
GuidePathFinder::commitPathToGuides(
    std::vector<frRect>& rects,
    const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map)
{
  std::vector<frBlockObject*> pins;
  pins.reserve(getPinCount());
  for (const auto& [pin, _] : pin_gcell_map) {
    pins.emplace_back(pin);
  }
  // find pin in which guide
  std::vector<std::vector<Point3D>> pin_to_gcell
      = getPinToGCellList(rects, pin_gcell_map, pins);
  int pin_idx = 0;
  for (auto& guides : pin_to_gcell) {
    if (guides.empty()) {
      logger_->error(DRT,
                     222,
                     "Pin {} is not visited by any guide",
                     getPinName(pins[pin_idx]));
    }
    ++pin_idx;
  }
  updateNodeMap(rects, pin_to_gcell);
  std::vector<std::pair<frBlockObject*, odb::Point>> gr_pins
      = getGRPins(pins, pin_to_gcell);
  clipGuides(rects);
  mergeGuides(rects);
  for (int i = 0; i < getGuideCount(); i++) {
    if (!visited_[i]) {
      continue;
    }
    auto& rect = rects[i];
    odb::Rect box = rect.getBBox();
    auto guide = std::make_unique<frGuide>();
    odb::Point begin = getDesign()->getTopBlock()->getGCellCenter(box.ll());
    odb::Point end = getDesign()->getTopBlock()->getGCellCenter(box.ur());
    guide->setPoints(begin, end);
    guide->setBeginLayerNum(rect.getLayerNum());
    guide->setEndLayerNum(rect.getLayerNum());
    guide->addToNet(net_);
    net_->addGuide(std::move(guide));
  }
  return gr_pins;
}

void GuidePathFinder::constructAdjList()
{
  adj_list_.resize(getNodeCount());
  for (const auto& [pt, indices] : node_map_) {
    const frLayerNum layer_num = pt.z();
    for (auto it1 = indices.begin(); it1 != indices.end(); it1++) {
      auto it2 = it1;
      it2++;
      auto idx1 = *it1;
      for (; it2 != indices.end(); it2++) {
        auto idx2 = *it2;
        // two pins, no edge
        if (isPinIdx(idx1) && isPinIdx(idx2)) {
          continue;
        }
        // two gcells, has edge
        if (!isPinIdx(idx1) && !isPinIdx(idx2)) {
          // no M1 cross-gcell routing allowed
          // BX200307: in general VIA_ACCESS_LAYER should not be used (instead
          // of 0)
          if (layer_num > router_cfg_->VIA_ACCESS_LAYERNUM) {
            adj_list_[idx1].push_back(idx2);
            adj_list_[idx2].push_back(idx1);
          }
        } else {
          // one pin, one gcell
          auto guide_idx = std::min(idx1, idx2);
          auto pin_idx = std::max(idx1, idx2);
          if (router_cfg_->ALLOW_PIN_AS_FEEDTHROUGH || isForceFeedThrough()) {
            adj_list_[pin_idx].push_back(guide_idx);
            adj_list_[guide_idx].push_back(pin_idx);
          } else {
            if (pin_idx == getGuideCount()) {
              // only out edge
              adj_list_[pin_idx].push_back(guide_idx);
            } else {
              // only in edge
              adj_list_[guide_idx].push_back(pin_idx);
            }
          }
        }
      }
      // add intersecting guide2guide edge excludes pin
      if (!isPinIdx(idx1)
          && node_map_.find({pt, layer_num + 2}) != node_map_.end()) {
        for (auto neighbor_idx : node_map_.at({pt, layer_num + 2})) {
          if (!isPinIdx(neighbor_idx)) {
            adj_list_[idx1].push_back(neighbor_idx);
            adj_list_[neighbor_idx].push_back(idx1);
          }
        }
      }
    }
  }
}

std::priority_queue<GuidePathFinder::Wavefront>
GuidePathFinder::getInitSearchQueue()
{
  std::priority_queue<Wavefront> queue;
  if (!visited_[getGuideCount()]) {
    // push only first pin into pq
    queue.push({getGuideCount(), -1, 0});
  } else {
    // push every visited node into pq
    for (int i = 0; i < getNodeCount(); i++) {
      if (is_on_path_[i]) {
        if (router_cfg_->ALLOW_PIN_AS_FEEDTHROUGH && isPinIdx(i)) {
          // TODO: set cost to 0
          queue.push({i, prev_idx_[i], 2});
        } else if (isForceFeedThrough() && isPinIdx(i)) {
          // penalize feedthrough in fallback mode
          queue.push({i, prev_idx_[i], 10});
        } else {
          queue.push({i, prev_idx_[i], 0});
        }
      }
    }
  }
  return queue;
}

bool GuidePathFinder::traverseGraph()
{
  for (int traversal_count = 0; traversal_count < getPinCount() - 1;
       traversal_count++) {
    std::priority_queue<Wavefront> queue = getInitSearchQueue();
    int visited_pin = -1;
    while (!queue.empty()) {
      auto curr_wavefront = queue.top();
      queue.pop();
      if (!is_on_path_[curr_wavefront.node_idx]
          && visited_[curr_wavefront.node_idx]) {
        continue;
      }
      if (curr_wavefront.node_idx > getGuideCount()
          && visited_[curr_wavefront.node_idx] == false) {
        visited_[curr_wavefront.node_idx] = true;
        prev_idx_[curr_wavefront.node_idx] = curr_wavefront.prev_idx;
        visited_pin = curr_wavefront.node_idx;
        break;
      }
      visited_[curr_wavefront.node_idx] = true;
      prev_idx_[curr_wavefront.node_idx] = curr_wavefront.prev_idx;
      // visit other nodes
      for (auto neighbor_idx : adj_list_[curr_wavefront.node_idx]) {
        if (!visited_[neighbor_idx]) {
          int cost = 1;
          if (!isPinIdx(neighbor_idx)) {
            cost += rects_[neighbor_idx].getBBox().dx()
                    + rects_[neighbor_idx].getBBox().dy();
          }
          queue.push({neighbor_idx,
                      curr_wavefront.node_idx,
                      curr_wavefront.cost + cost});
        }
      }
    }
    // trace back path
    int last_visited_idx = visited_pin;
    while ((last_visited_idx != -1) && (!is_on_path_[last_visited_idx])) {
      is_on_path_[last_visited_idx] = true;
      last_visited_idx = prev_idx_[last_visited_idx];
    }
    visited_ = is_on_path_;
  }
  // skip one-pin net
  if (getPinCount() == 1) {
    return true;
  }
  int visited_pin_count
      = count(visited_.begin() + guide_count_, visited_.end(), true);
  const bool success = (visited_pin_count == getPinCount());
  return success;
}

void GuidePathFinder::bfs(int node_idx)
{
  visited_.clear();
  visited_.resize(getNodeCount(), false);
  std::queue<Wavefront> queue;
  queue.push({node_idx, -1, 0});
  while (!queue.empty()) {
    auto curr_wavefront = queue.front();
    queue.pop();
    visited_[curr_wavefront.node_idx] = true;
    for (auto neighbor_idx : adj_list_[curr_wavefront.node_idx]) {
      if (!visited_[neighbor_idx]) {
        queue.push(
            {neighbor_idx, curr_wavefront.node_idx, curr_wavefront.cost + 1});
      }
    }
  }
}

void GuidePathFinder::connectDisconnectedComponents(
    const std::vector<frRect>& rects,
    TrackIntervalsByLayer& intvs)
{
  const int unvisited_pin_idx
      = std::distance(visited_.begin(),
                      std::find_if(visited_.begin() + getGuideCount() + 1,
                                   visited_.end(),
                                   [](bool x) { return !x; }));
  auto getVisitedIndices = [this]() {
    std::vector<int> indices;
    for (int i = 0; i < getGuideCount(); i++) {
      if (visited_[i]) {
        indices.push_back(i);
      }
    }
    return indices;
  };
  bfs(getGuideCount());
  const auto component1 = getVisitedIndices();
  bfs(unvisited_pin_idx);
  const auto component2 = getVisitedIndices();
  BridgeGuide best_bridge;

  for (const auto idx1 : component1) {
    for (const auto idx2 : component2) {
      if (rects[idx1].getLayerNum() != rects[idx2].getLayerNum()) {
        continue;
      }
      const auto layer_num = rects[idx1].getLayerNum();
      const bool is_horizontal = getTech()->getLayer(layer_num)->isHorizontal();
      const frCoord track_idx1 = is_horizontal ? rects[idx1].getBBox().yMin()
                                               : rects[idx1].getBBox().xMin();
      const frCoord track_idx2 = is_horizontal ? rects[idx2].getBBox().yMin()
                                               : rects[idx2].getBBox().xMin();
      if (std::abs(track_idx1 - track_idx2) > 1) {
        continue;
      }

      const frCoord begin_idx1 = is_horizontal ? rects[idx1].getBBox().xMin()
                                               : rects[idx1].getBBox().yMin();
      const frCoord end_idx1 = is_horizontal ? rects[idx1].getBBox().xMax()
                                             : rects[idx1].getBBox().yMax();
      const frCoord begin_idx2 = is_horizontal ? rects[idx2].getBBox().xMin()
                                               : rects[idx2].getBBox().yMin();
      const frCoord end_idx2 = is_horizontal ? rects[idx2].getBBox().xMax()
                                             : rects[idx2].getBBox().yMax();

      const BridgeGuide bridge1(track_idx1, end_idx1, begin_idx2, layer_num);
      const BridgeGuide bridge2(track_idx2, end_idx2, begin_idx1, layer_num);
      best_bridge = std::min(best_bridge, bridge1);
      best_bridge = std::min(best_bridge, bridge2);
    }
  }
  if (best_bridge.layer_num == -1) {
    return;
  }
  intvs[best_bridge.layer_num][best_bridge.track_idx].insert(
      Interval::closed(best_bridge.begin_idx, best_bridge.end_idx));
  addTouchingGuidesBridges(intvs, logger_);
}
void GuideProcessor::saveGuidesUpdates()
{
  auto block = db_->getChip()->getBlock();
  auto dbTech = db_->getTech();
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    auto dbNet = block->findNet(net->getName().c_str());
    dbNet->clearGuides();
    for (auto& guide : net->getGuides()) {
      auto [bp, ep] = guide->getPoints();
      odb::Point bpIdx = getDesign()->getTopBlock()->getGCellIdx(bp);
      odb::Point epIdx = getDesign()->getTopBlock()->getGCellIdx(ep);
      odb::Rect bbox = getDesign()->getTopBlock()->getGCellBox(bpIdx);
      odb::Rect ebox = getDesign()->getTopBlock()->getGCellBox(epIdx);
      frLayerNum bNum = guide->getBeginLayerNum();
      frLayerNum eNum = guide->getEndLayerNum();
      if (bNum != eNum) {
        for (auto lNum = std::min(bNum, eNum); lNum <= std::max(bNum, eNum);
             lNum += 2) {
          auto layer = getTech()->getLayer(lNum);
          auto dbLayer = dbTech->findLayer(layer->getName().c_str());
          odb::dbGuide::create(
              dbNet,
              dbLayer,
              dbLayer,
              {bbox.xMin(), bbox.yMin(), ebox.xMax(), ebox.yMax()},
              false);
        }
      } else {
        auto layerName = getTech()->getLayer(bNum)->getName();
        auto dbLayer = dbTech->findLayer(layerName.c_str());
        odb::dbGuide::create(
            dbNet,
            dbLayer,
            dbLayer,
            {bbox.xMin(), bbox.yMin(), ebox.xMax(), ebox.yMax()},
            false);
      }
    }
    auto dbGuides = dbNet->getGuides();
    if (dbGuides.orderReversed() && dbGuides.reversible()) {
      dbGuides.reverse();
    }
  }
}

void GuideProcessor::processGuides()
{
  if (tmp_guides_.empty()) {
    return;
  }
  frTime t;
  ProfileTask profile("IO:postProcessGuide");
  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 169, "Post process guides.");
  }
  buildGCellPatterns();

  getDesign()->getRegionQuery()->initOrigGuide(tmp_guides_);
  int cnt = 0;
  std::vector<std::pair<frNet*, std::vector<frRect>>> nets_to_guides;
  frBlockObjectMap<std::vector<std::pair<frBlockObject*, odb::Point>>>
      net_to_gr_pins;
  for (auto& [net, rects] : tmp_guides_) {
    nets_to_guides.push_back({net, rects});
    net->setOrigGuides(rects);
    net_to_gr_pins[net];
  }
  omp_set_num_threads(router_cfg_->MAX_THREADS);
  utl::ThreadException exception;
#pragma omp parallel for
  for (int i = 0; i < nets_to_guides.size(); i++) {
    try {
      net_to_gr_pins[nets_to_guides[i].first]
          = genGuides(nets_to_guides[i].first, nets_to_guides[i].second);

#pragma omp critical
      {
        cnt++;
        if (router_cfg_->VERBOSE > 0) {
          if (cnt < 1000000) {
            if (cnt % 100000 == 0) {
              logger_->report("  complete {} nets.", cnt);
            }
          } else {
            if (cnt % 1000000 == 0) {
              logger_->report("  complete {} nets.", cnt);
            }
          }
        }
      }
    } catch (...) {
      exception.capture();
    }
  }
  exception.rethrow();
  std::vector<std::pair<frBlockObject*, odb::Point>> all_gr_pins;
  for (auto [_, gr_pins] : net_to_gr_pins) {
    all_gr_pins.insert(all_gr_pins.end(), gr_pins.begin(), gr_pins.end());
  }

  // global unique id for guides
  int currId = 0;
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    for (auto& guide : net->getGuides()) {
      guide->setId(currId);
      currId++;
    }
  }

  logger_->info(DRT, 178, "Init guide query.");
  getDesign()->getRegionQuery()->initGuide();
  getDesign()->getRegionQuery()->printGuide();
  logger_->info(DRT, 179, "Init gr pin query.");
  getDesign()->getRegionQuery()->initGRPin(all_gr_pins);

  if (router_cfg_->VERBOSE > 0) {
    t.print(logger_);
  }
  if (!router_cfg_->SAVE_GUIDE_UPDATES) {
    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT, 245, "skipped writing guide updates to database.");
    }
  } else {
    saveGuidesUpdates();
  }
}

}  // namespace drt::io
