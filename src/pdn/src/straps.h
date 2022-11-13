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

#include "grid_component.h"
#include "pdn/PdnGen.hh"

namespace odb {
class dbBox;
class dbITerm;
class dbTechLayer;
}  // namespace odb

namespace pdn {
class Grid;

class Straps : public GridComponent
{
 public:
  Straps(Grid* grid,
         odb::dbTechLayer* layer,
         int width,
         int pitch,
         int spacing = 0,
         int number_of_straps = 0);

  void setOffset(int offset);
  int getOffset() const { return offset_; }
  void setSnapToGrid(bool snap);

  void setExtend(ExtensionMode mode);
  void setStrapStartEnd(int start, int end);
  int getStrapStart() const { return strap_start_; }
  int getStrapEnd() const { return strap_end_; }

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  odb::dbTechLayer* getLayer() const { return layer_; }
  void setLayer(odb::dbTechLayer* layer) { layer_ = layer; }
  int getWidth() const { return width_; }
  void setWidth(int width) { width_ = width; }
  int getSpacing() const { return spacing_; }
  void setSpacing(int spacing) { spacing_ = spacing; }
  int getPitch() const { return pitch_; }
  void setPitch(int pitch) { pitch_ = pitch; }
  ExtensionMode getExtendMode() const { return extend_mode_; }
  void setDirection(odb::dbTechLayerDir direction) { direction_ = direction; }
  odb::dbTechLayerDir getDirection() const { return direction_; }
  bool isHorizontal() const
  {
    return direction_ == odb::dbTechLayerDir::HORIZONTAL;
  }

  virtual void report() const override;
  virtual Type type() const override { return GridComponent::Strap; }

  virtual void checkLayerSpecifications() const override;

  // get the width of a set of straps, useful when estimating the shape of the
  // straps.
  int getStrapGroupWidth() const;

 protected:
  bool checkLayerOffsetSpecification(bool error = false) const;

 private:
  odb::dbTechLayer* layer_;
  int width_;
  int spacing_;
  int pitch_;
  int offset_;
  int number_of_straps_;
  odb::dbTechLayerDir direction_;
  bool snap_;
  ExtensionMode extend_mode_;
  int strap_start_;
  int strap_end_;

  void makeStraps(int x_start,
                  int y_start,
                  int x_end,
                  int y_end,
                  bool is_delta_x,
                  const TechLayer& layer,
                  const ShapeTree& avoid);
};

class FollowPins : public Straps
{
 public:
  FollowPins(Grid* grid, odb::dbTechLayer* layer, int width);

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  virtual Type type() const override { return GridComponent::Followpin; }

  virtual void checkLayerSpecifications() const override;

 private:
  // search for the shape of the power pins in the standard cells to determine
  // the width if possible
  void determineWidth();
};

class PadDirectConnectionStraps : public Straps
{
 public:
  enum ConnectionType
  {
    None,
    Edge,
    OverPads
  };

  PadDirectConnectionStraps(
      Grid* grid,
      odb::dbITerm* iterm,
      const std::vector<odb::dbTechLayer*>& connect_pad_layers);

  // true if the iterm can be connected to a ring
  bool canConnect() const;

  virtual void makeShapes(const ShapeTreeMap& other_shapes) override;

  virtual void report() const override;
  virtual Type type() const override { return GridComponent::PadConnect; }

  // disable layer spec checks
  virtual void checkLayerSpecifications() const override {}

  odb::dbITerm* getITerm() const { return iterm_; }

  virtual void getConnectableShapes(ShapeTreeMap& shapes) const override;

  // cut shapes and remove if connection to ring is not possible
  virtual void cutShapes(const ShapeTreeMap& obstructions) override;

  void setConnectionType(ConnectionType type);
  ConnectionType getConnectionType() const { return type_; }

  static void unifyConnectionTypes(
      const std::set<PadDirectConnectionStraps*>& straps);

 private:
  odb::dbITerm* iterm_;
  odb::dbWireShapeType target_shapes_;
  odb::dbDirection pad_edge_;
  ConnectionType type_;
  std::vector<odb::dbTechLayer*> layers_;

  std::vector<odb::dbBox*> pins_;

  std::string getName() const;

  // find all the dbBox's to attempt to connect to
  void initialize(ConnectionType type);

  bool isConnectHorizontal() const;

  std::vector<odb::dbBox*> getPinsFacingCore();
  std::vector<odb::dbBox*> getPinsFormingRing();
  std::map<odb::dbTechLayer*, std::vector<odb::dbBox*>> getPinsByLayer() const;

  void makeShapesFacingCore(const ShapeTreeMap& other_shapes);
  void makeShapesOverPads(const ShapeTreeMap& other_shapes);

  std::vector<PadDirectConnectionStraps*> getAssociatedStraps() const;
  const std::vector<odb::dbBox*>& getPins() const { return pins_; }

  ShapePtr getClosestShape(const ShapeTree& search_shapes,
                           const odb::Rect& pin_shape,
                           odb::dbNet* net) const;
};

class RepairChannelStraps : public Straps
{
 public:
  RepairChannelStraps(Grid* grid,
                      Straps* target,
                      odb::dbTechLayer* connect_to,
                      const ShapeTreeMap& other_shapes,
                      const std::set<odb::dbNet*>& nets,
                      const odb::Rect& area,
                      const odb::Rect& obs_check_area);

  virtual Type type() const override { return GridComponent::RepairChannel; }

  virtual void report() const override;

  // returns only the nets that need to be repaired.
  virtual std::vector<odb::dbNet*> getNets() const override;

  // cut shapes and remove any segments outside of the area
  virtual void cutShapes(const ShapeTreeMap& obstructions) override;

  bool isRepairValid() const { return !invalid_; }

  bool isAtEndOfRepairOptions() const;
  void continueRepairs(const ShapeTreeMap& other_shapes);
  bool testBuild(const ShapeTreeMap& local_shapes,
                 const ShapeTreeMap& obstructions);
  bool isEmpty() const;

  void addNets(const std::set<odb::dbNet*> nets)
  {
    nets_.insert(nets.begin(), nets.end());
  }
  const odb::Rect& getArea() const { return area_; }

  // static functions to help build repair channels
  // repair unconnected straps by adding repair channel straps
  static void repairGridChannels(Grid* grid,
                                 const ShapeTreeMap& global_shapes,
                                 ShapeTreeMap& obstructions,
                                 bool allow);

  struct RepairChannelArea
  {
    odb::Rect area;
    odb::Rect obs_area;
    Straps* target;
    odb::dbTechLayer* connect_to;
    std::set<odb::dbNet*> nets;
  };
  // find all straps in grid that are not connected for anything
  static std::vector<RepairChannelArea> findRepairChannels(Grid* grid);

 private:
  std::set<odb::dbNet*> nets_;
  odb::dbTechLayer* connect_to_;
  odb::Rect area_;
  odb::Rect obs_check_area_;

  bool invalid_;

  // search for the right width, spacing, and offset to connect to the channel
  void determineParameters(const ShapeTreeMap& obstructions);
  bool determineOffset(const ShapeTreeMap& obstructions,
                       int extra_offset = 0,
                       int bisect_dist = 0,
                       int level = 0);

  std::string getNetString() const;

  static std::vector<RepairChannelArea> findRepairChannels(
      Grid* grid,
      const ShapeTree& shapes,
      odb::dbTechLayer* layer);
  static Straps* getTargetStrap(Grid* grid, odb::dbTechLayer* layer);
  static odb::dbTechLayer* getHighestStrapLayer(Grid* grid);

  int getNextWidth() const;
  int getMaxLength() const;
};

}  // namespace pdn
