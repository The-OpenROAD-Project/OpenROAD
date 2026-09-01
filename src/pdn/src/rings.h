// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <utility>
#include <vector>

#include "grid.h"
#include "grid_component.h"
#include "odb/geom.h"
#include "shape.h"

namespace odb {
class dbTechLayer;
}  // namespace odb

namespace pdn {

class Rings : public GridComponent
{
 public:
  struct Layer
  {
    odb::dbTechLayer* layer = nullptr;
    int width = 0;
    int spacing = 0;
  };

  Rings(Grid* grid, const Layer& layer0, const Layer& layer1);

  // the offset is given in the grid's as-drawn frame and is remapped onto the
  // placed instance; offset_ is always in the placed frame
  void setOffset(const EdgeSpec& offset);
  const EdgeSpec& getOffset() const { return offset_; }
  void setPadOffset(const EdgeSpec& offset);

  void setExtendToBoundary(bool value);
  void setAllowOutsideDieArea() { allow_outside_die_ = true; }

  // generate the rings
  void makeShapes(const Shape::ShapeTreeMap& other_shapes) override;

  std::vector<odb::dbTechLayer*> getLayers() const;

  // returns the horizontal and vertical widths of the rings, useful when
  // estimating the ring size.
  void getTotalWidth(int& hor, int& ver) const;

  void report() const override;
  Type type() const override { return GridComponent::kRing; }

  void checkLayerSpecifications() const override;

 protected:
  bool areIntersectionsAllowed() const override
  {
    return layer0_.layer == layer1_.layer;
  }

 private:
  Layer layer0_;
  Layer layer1_;
  EdgeSpec offset_;
  bool extend_to_boundary_ = false;
  bool allow_outside_die_ = false;

  void checkDieArea() const;
  // X and Y overrun of the ring past the die, per side of the core
  std::pair<int, int> getDieAreaDeficit(const Region& die_area) const;

  // The inner boundary of the ring: the domain outline pushed out by the
  // offsets.  getInnerRingRect() is its bounding box, which is what the
  // rectangular shape builder works from.
  Region getInnerRingOutline() const;
  odb::Rect getInnerRingRect() const;
};

}  // namespace pdn
