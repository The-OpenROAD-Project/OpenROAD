// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <vector>

#include "grid_component.h"
#include "odb/geom.h"
#include "shape.h"

namespace odb {
class dbTechLayer;
}  // namespace odb

namespace pdn {
class Grid;

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

  void setOffset(const std::array<int, 4>& offset);
  const std::array<int, 4>& getOffset() const { return offset_; }
  void setPadOffset(const std::array<int, 4>& offset);

  void setExtendToBoundary(bool value);
  void setAllowOutsideDieArea() { allow_outside_die_ = true; }

  // generate the rings
  void makeShapes(const Shape::ShapeTreeMap& other_shapes) override;

  std::vector<odb::dbTechLayer*> getLayers() const;

  // returns the horizontal and vertical widths of the rings, useful when
  // estimating the ring size.
  void getTotalWidth(int& hor, int& ver) const;

  void report() const override;
  Type type() const override { return GridComponent::Ring; }

  void checkLayerSpecifications() const override;

 protected:
  bool areIntersectionsAllowed() const override
  {
    return layer0_.layer == layer1_.layer;
  }

 private:
  Layer layer0_;
  Layer layer1_;
  std::array<int, 4> offset_ = {0, 0, 0, 0};
  bool extend_to_boundary_ = false;
  bool allow_outside_die_ = false;

  void checkDieArea() const;

  odb::Rect getInnerRingOutline() const;
};

}  // namespace pdn
