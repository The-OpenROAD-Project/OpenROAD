// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/geom.h"

namespace web {

// A gui::Painter that collects the rectangles and polygons a
// descriptor->highlight() draws, for the overlay renderer to paint.
//
// `budget` bounds what one object may accumulate.  It matters because a single
// descriptor can emit one shape per leaf: DbGroupDescriptor::highlight recurses
// into subgroups and calls the instance descriptor per member, so collecting a
// root cluster of a million-instance design would allocate a million odb::Rect
// before the caller got to decide the set is too big to send.  Past the budget
// the shapes are dropped and only their union is kept, which is exactly what
// the caller falls back to anyway — so nothing is lost and the peak is bounded.
class ShapeCollector : public gui::Painter
{
 public:
  explicit ShapeCollector(size_t budget = std::numeric_limits<size_t>::max())
      : Painter(nullptr, odb::Rect(), 1.0), budget_(budget)
  {
    overflow_bbox.mergeInit();
  }

  std::vector<odb::Rect> rects;
  std::vector<odb::Polygon> polys;
  // Union of everything dropped for exceeding the budget; inverted while
  // nothing was dropped.
  odb::Rect overflow_bbox;

  bool overflowed() const { return !overflow_bbox.isInverted(); }

  // Ready the collector for another object, with a new budget.  Resetting
  // through one call is what keeps a reusing caller from clearing the shape
  // vectors and forgetting the overflow box.
  void reset(size_t budget)
  {
    rects.clear();
    polys.clear();
    overflow_bbox.mergeInit();
    budget_ = budget;
  }

  // The union of everything this object produced, kept and dropped alike.
  odb::Rect unionBBox() const
  {
    odb::Rect bbox = overflow_bbox;
    for (const odb::Rect& r : rects) {
      bbox.merge(r);
    }
    for (const odb::Polygon& poly : polys) {
      bbox.merge(poly.getEnclosingRect());
    }
    return bbox;
  }

  void drawRect(const odb::Rect& rect, int, int) override
  {
    if (atBudget()) {
      overflow_bbox.merge(rect);
      return;
    }
    rects.push_back(rect);
  }
  void drawPolygon(const odb::Polygon& polygon) override
  {
    if (atBudget()) {
      overflow_bbox.merge(polygon.getEnclosingRect());
      return;
    }
    polys.push_back(polygon);
  }
  void drawOctagon(const odb::Oct& oct) override
  {
    drawPolygon(odb::Polygon(oct));
  }

  // No-ops
  Color getPenColor() override { return {}; }
  void setPen(odb::dbTechLayer*, bool) override {}
  void setPen(const Color&, bool, int) override {}
  void setPenWidth(int) override {}
  void setBrush(odb::dbTechLayer*, int) override {}
  void setBrush(const Color&, const Brush&) override {}
  void setFont(const Font&) override {}
  void saveState() override {}
  void restoreState() override {}
  void drawLine(const odb::Point&, const odb::Point&) override {}
  void drawCircle(int, int, int) override {}
  void drawX(int, int, int) override {}
  void drawPolygon(const std::vector<odb::Point>&) override {}
  void drawString(int, int, Anchor, const std::string&, bool) override {}
  odb::Rect stringBoundaries(int, int, Anchor, const std::string&) override
  {
    return {};
  }
  void drawRuler(int, int, int, int, bool, const std::string&) override {}

 private:
  bool atBudget() const { return rects.size() + polys.size() >= budget_; }

  size_t budget_;
};

}  // namespace web
