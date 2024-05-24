///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <array>
#include <vector>

#include "grid_component.h"

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

  Rings(Grid* grid, const std::array<Layer, 2>& layers);

  void setOffset(const std::array<int, 4>& offset);
  const std::array<int, 4>& getOffset() const { return offset_; }
  void setPadOffset(const std::array<int, 4>& offset);

  void setExtendToBoundary(bool value);

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
    return layers_[0].layer == layers_[1].layer;
  }

 private:
  std::array<Layer, 2> layers_;
  std::array<int, 4> offset_ = {0, 0, 0, 0};
  bool extend_to_boundary_ = false;

  void checkDieArea() const;

  odb::Rect getInnerRingOutline() const;
};

}  // namespace pdn
