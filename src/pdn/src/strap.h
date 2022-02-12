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
#include <set>

#include "grid_shape.h"

namespace odb {
class dbBox;
class dbITerm;
class dbTechLayer;
}  // namespace odb

namespace pdn {
class Grid;

class Strap : public GridShape
{
 public:
  Strap(Grid* grid,
        odb::dbTechLayer* layer,
        int width,
        int pitch,
        int spacing = 0,
        int number_of_straps = 0);
  ~Strap() {}

  void setOffset(int offset);
  int getOffset() const { return offset_; }
  void setSnapToGrid(bool snap);

  enum Extend
  {
    CORE,
    RINGS,
    BOUNDARY
  };
  void setExtend(Extend mode);

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  odb::dbTechLayer* getLayer() const { return layer_; }
  int getWidth() const { return width_; }
  void setWidth(int width) { width_ = width; }
  int getSpacing() const { return spacing_; }
  void setSpacing(int spacing) { spacing_ = spacing; }
  Extend getExtendMode() const { return extend_mode_; }
  void setDirection(odb::dbTechLayerDir direction) { direction_ = direction; }
  odb::dbTechLayerDir getDirection() const { return direction_; }
  void setStartWithPower(bool value) { starts_with_power_ = value; }
  bool getStartsWithPower() const { return starts_with_power_; }
  bool getStartsWithGround() const { return !getStartsWithPower(); }

  // returns the order in which to lay out the straps
  virtual std::vector<odb::dbNet*> getNets() const;

  virtual void report() const override;
  virtual Type type() const override { return GridShape::Strap; }

  virtual void checkLayerSpecifications() const override;

  // get the width of a set of straps, useful when estimating the shape of the straps.
  int getStrapGroupWidth() const;

 private:
  odb::dbTechLayer* layer_;
  int width_;
  int spacing_;
  int pitch_;
  int offset_;
  int number_of_straps_;
  odb::dbTechLayerDir direction_;
  bool snap_;
  bool starts_with_power_;
  Extend extend_mode_;

  // returns the track to snap the strap too
  int snapToGrid(int pos, const std::vector<int>& grid, int greater_than = 0) const;
  void makeStraps(int x_start,
                  int y_start,
                  int x_end,
                  int y_end,
                  bool is_delta_x,
                  const std::vector<int>& snap_grid);
};

class FollowPin : public Strap
{
 public:
  FollowPin(Grid* grid, odb::dbTechLayer* layer, int width);
  ~FollowPin() {}

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  virtual Type type() const override { return GridShape::Followpin; }

  virtual void checkLayerSpecifications() const override;

 private:
  // search for the shape of the power pins in the standard cells to determine the width if possible
  void determineWidth();
};

class PadDirectConnect : public Strap
{
 public:
  PadDirectConnect(Grid* grid,
                   odb::dbITerm* iterm,
                   const std::vector<odb::dbTechLayer*>& connect_pad_layers);
  ~PadDirectConnect() {}

  // true if the iterm can be connected to a ring
  bool canConnect() const;

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  virtual void report() const override;
  virtual Type type() const override { return GridShape::PadConnect; }

 private:
  odb::dbITerm* iterm_;
  odb::dbWireShapeType target_shapes_;
  odb::dbDirection pad_edge_;

  std::vector<odb::dbBox*> pins_;

  std::string getName() const;

  // find all the dbBox's to attempt to connect to
  void initialize(const std::vector<odb::dbTechLayer*>& layers);

  bool isConnectHorizontal() const;
};

class RepairChannel : public Strap
{
 public:
  RepairChannel(Grid* grid,
                Strap* target,
                odb::dbTechLayer* connect_to,
                const ShapeTreeMap& other_shapes,
                const std::set<odb::dbNet*>& nets,
                const odb::Rect& area);
  ~RepairChannel() {}

  virtual Type type() const override { return GridShape::RepairChannel; }

  // returns only the nets that need to be repaired.
  virtual std::vector<odb::dbNet*> getNets() const override;

 private:
  std::set<odb::dbNet*> nets_;
  odb::dbTechLayer* connect_to_;

  // search for the right width, spacing, and offset to connect to the channel
  void determineParameters(const odb::Rect& area,
                           const ShapeTreeMap& obstructions);
  bool determineOffset(const odb::Rect& area,
                       const ShapeTreeMap& obstructions,
                       int extra_offset = 0,
                       int bisect_dist = 0,
                       int level = 0);

  std::string getNetString() const;
};

}  // namespace pdn
