// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "db/gcObj/gcShape.h"
#include "db/obj/frMarker.h"
#include "db/tech/frConstraint.h"
#include "frBaseTypes.h"
#include "gc/FlexGC.h"
#include "gc/FlexGC_impl.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

odb::Rect FlexGCWorker::Impl::checkForbiddenSpc_queryBox(gcRect* rect,
                                                         frCoord minSpc,
                                                         frCoord maxSpc,
                                                         bool isH,
                                                         bool right)
{
  odb::Rect queryBox(
      gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  if (isH) {
    if (right) {
      queryBox.set_ylo(gtl::yh(*rect) + minSpc);
      queryBox.set_yhi(gtl::yh(*rect) + maxSpc);
    } else {
      queryBox.set_ylo(gtl::yl(*rect) - maxSpc);
      queryBox.set_yhi(gtl::yl(*rect) - minSpc);
    }
  } else {
    if (right) {
      queryBox.set_xlo(gtl::xh(*rect) + minSpc);
      queryBox.set_xhi(gtl::xh(*rect) + maxSpc);
    } else {
      queryBox.set_xlo(gtl::xl(*rect) - maxSpc);
      queryBox.set_xhi(gtl::xl(*rect) - minSpc);
    }
  }
  return queryBox;
}

void FlexGCWorker::Impl::checkForbiddenSpc_main(
    gcRect* rect1,
    gcRect* rect2,
    frLef58ForbiddenSpcConstraint* con,
    bool isH)
{
  gtl::rectangle_data<frCoord> markerRect(*rect1);
  gtl::generalized_intersect(markerRect, *rect2);
  auto prlX = gtl::delta(markerRect, gtl::HORIZONTAL);
  auto prlY = gtl::delta(markerRect, gtl::VERTICAL);
  auto distX = gtl::euclidean_distance(*rect1, *rect2, gtl::HORIZONTAL);
  auto distY = gtl::euclidean_distance(*rect1, *rect2, gtl::VERTICAL);
  // skip short violations
  if (distX == 0 && distY == 0) {
    return;
  }
  if (distX) {
    prlX = -prlX;
  }
  if (distY) {
    prlY = -prlY;
  }
  if (!con->isPrlValid(isH ? prlX : prlY)) {
    return;
  }
  if (!con->isForbiddenSpc(isH ? distY : distX)) {
    return;
  }
  std::vector<rq_box_value_t<gcRect*>> result;
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.queryMaxRectangle(markerRect, rect1->getLayerNum(), result);
  for (auto& [objBox, ptr] : result) {
    if (ptr == rect1 || ptr == rect2) {
      continue;
    }
    if (gtl::intersects(*ptr, *rect1) || gtl::intersects(*ptr, *rect2)) {
      // same metal or short
      continue;
    }
    gtl::polygon_90_set_data<frCoord> tmpPoly;
    using boost::polygon::operators::operator+=;
    using boost::polygon::operators::operator&=;
    tmpPoly += markerRect;
    tmpPoly &= *ptr;  // tmpPoly now is widthrect
    if (gtl::area(tmpPoly) == 0) {
      continue;
    }
    if (rect1->isFixed() && rect2->isFixed() && ptr->isFixed()) {
      continue;
    }
    if (!hasRoute(rect1, markerRect) && !hasRoute(rect2, markerRect)
        && !hasRoute(ptr, markerRect)) {
      continue;
    }
    // add violation
    auto marker = std::make_unique<frMarker>();
    odb::Rect box(gtl::xl(markerRect),
                  gtl::yl(markerRect),
                  gtl::xh(markerRect),
                  gtl::yh(markerRect));
    marker->setBBox(box);
    marker->setLayerNum(rect1->getLayerNum());
    marker->setConstraint(con);
    marker->addSrc(rect1->getNet()->getOwner());
    marker->addVictim(rect1->getNet()->getOwner(),
                      std::make_tuple(rect1->getLayerNum(),
                                      odb::Rect(gtl::xl(*rect1),
                                                gtl::yl(*rect1),
                                                gtl::xh(*rect1),
                                                gtl::yh(*rect1)),
                                      rect1->isFixed()));
    marker->addSrc(rect2->getNet()->getOwner());
    marker->addAggressor(rect2->getNet()->getOwner(),
                         std::make_tuple(rect2->getLayerNum(),
                                         odb::Rect(gtl::xl(*rect2),
                                                   gtl::yl(*rect2),
                                                   gtl::xh(*rect2),
                                                   gtl::yh(*rect2)),
                                         rect2->isFixed()));
    marker->addSrc(ptr->getNet()->getOwner());
    marker->addAggressor(
        ptr->getNet()->getOwner(),
        std::make_tuple(
            ptr->getLayerNum(),
            odb::Rect(
                gtl::xl(*ptr), gtl::yl(*ptr), gtl::xh(*ptr), gtl::yh(*ptr)),
            ptr->isFixed()));
    addMarker(std::move(marker));
  }
}

bool FlexGCWorker::Impl::checkForbiddenSpc_twoedges(
    gcRect* rect,
    frLef58ForbiddenSpcConstraint* con,
    bool isH)
{
  odb::Rect queryBox;
  auto hasWireInWithin
      = [](gcRect* rect, const std::vector<rq_box_value_t<gcRect*>>& result) {
          for (auto& [objBox, ptr] : result) {
            if (rect == ptr) {
              continue;
            }
            return true;
          }
          return false;
        };
  std::vector<rq_box_value_t<gcRect*>> result;
  auto& workerRegionQuery = getWorkerRegionQuery();
  queryBox = checkForbiddenSpc_queryBox(
      rect, 0, con->getTwoEdgesWithin(), isH, true);
  workerRegionQuery.queryMaxRectangle(queryBox, rect->getLayerNum(), result);
  if (!hasWireInWithin(rect, result)) {
    return false;
  }
  result.clear();
  queryBox = checkForbiddenSpc_queryBox(
      rect, 0, con->getTwoEdgesWithin(), isH, false);
  workerRegionQuery.queryMaxRectangle(queryBox, rect->getLayerNum(), result);
  return hasWireInWithin(rect, result);
}
void FlexGCWorker::Impl::checkForbiddenSpc_main(
    gcRect* rect,
    frLef58ForbiddenSpcConstraint* con)
{
  if (!con->isWidthValid(rect->width()) || !con->isPrlValid(rect->length())) {
    return;
  }
  bool isH = getTech()->getLayer(rect->getLayerNum())->getDir()
             == odb::dbTechLayerDir::HORIZONTAL;
  if (rect->width() != rect->length()) {
    isH = gtl::delta(*rect, gtl::orientation_2d_enum::HORIZONTAL)
          > gtl::delta(*rect, gtl::orientation_2d_enum::VERTICAL);
  }
  // check TWOEDGES
  if (!checkForbiddenSpc_twoedges(rect, con, isH)) {
    return;
  }
  odb::Rect queryBox;
  std::vector<rq_box_value_t<gcRect*>> result;
  auto& workerRegionQuery = getWorkerRegionQuery();

  queryBox = checkForbiddenSpc_queryBox(
      rect, con->getMinSpc(), con->getMaxSpc(), isH, true);
  workerRegionQuery.queryMaxRectangle(queryBox, rect->getLayerNum(), result);

  queryBox = checkForbiddenSpc_queryBox(
      rect, con->getMinSpc(), con->getMaxSpc(), isH, false);
  workerRegionQuery.queryMaxRectangle(queryBox, rect->getLayerNum(), result);
  // result.insert(result.end(), tmpResult.begin(), tmpResult.end());
  for (auto& [objBox, ptr] : result) {
    if (rect == ptr) {
      continue;
    }
    if (ptr->width() != ptr->length()) {
      bool isPtrH = gtl::delta(*ptr, gtl::orientation_2d_enum::HORIZONTAL)
                    > gtl::delta(*ptr, gtl::orientation_2d_enum::VERTICAL);
      if (isPtrH != isH) {
        continue;
      }
    }
    checkForbiddenSpc_main(rect, ptr, con, isH);
  }
}

}  // namespace drt
