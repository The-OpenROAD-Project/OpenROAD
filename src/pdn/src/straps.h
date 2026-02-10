// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "grid_component.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "pdn/PdnGen.hh"
#include "shape.h"

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

  void makeShapes(const Shape::ShapeTreeMap& other_shapes) override;

  odb::dbTechLayer* getLayer() const { return layer_; }
  void setLayer(odb::dbTechLayer* layer) { layer_ = layer; }
  int getWidth() const { return width_; }
  void setWidth(int width) { width_ = width; }
  int getSpacing() const { return spacing_; }
  void setSpacing(int spacing) { spacing_ = spacing; }
  int getPitch() const { return pitch_; }
  void setPitch(int pitch) { pitch_ = pitch; }
  ExtensionMode getExtendMode() const { return extend_mode_; }
  void setDirection(const odb::dbTechLayerDir& direction)
  {
    direction_ = direction;
  }
  odb::dbTechLayerDir getDirection() const { return direction_; }
  bool isHorizontal() const
  {
    return direction_ == odb::dbTechLayerDir::HORIZONTAL;
  }
  void setAllowOutsideCoreArea(bool allow_out_of_core)
  {
    allow_out_of_core_ = allow_out_of_core;
  }

  void report() const override;
  Type type() const override { return GridComponent::Strap; }

  void checkLayerSpecifications() const override;

  // get the width of a set of straps, useful when estimating the shape of the
  // straps.
  int getStrapGroupWidth() const;

 protected:
  bool checkLayerOffsetSpecification(bool error = false) const;
  std::string getNetString() const;

 private:
  odb::dbTechLayer* layer_;
  int width_;
  int spacing_;
  int pitch_;
  int offset_ = 0;
  int number_of_straps_;
  odb::dbTechLayerDir direction_;
  bool snap_ = false;
  ExtensionMode extend_mode_ = ExtensionMode::CORE;
  int strap_start_ = 0;
  int strap_end_ = 0;
  bool allow_out_of_core_ = false;

  void makeStraps(int x_start,
                  int y_start,
                  int x_end,
                  int y_end,
                  int abs_start,
                  int abs_end,
                  bool is_delta_x,
                  const TechLayer& layer,
                  const Shape::ObstructionTree& avoid);
};

class FollowPins : public Straps
{
 public:
  FollowPins(Grid* grid, odb::dbTechLayer* layer, int width);

  void makeShapes(const Shape::ShapeTreeMap& other_shapes) override;

  Type type() const override { return GridComponent::Followpin; }

  void checkLayerSpecifications() const override;

 private:
  // search for the shape of the power pins in the standard cells to determine
  // the width if possible
  void determineWidth();
};

class PadDirectConnectionStraps : public Straps
{
 public:
  PadDirectConnectionStraps(
      Grid* grid,
      odb::dbITerm* iterm,
      const std::vector<odb::dbTechLayer*>& connect_pad_layers);

  // true if the iterm can be connected to a ring
  bool canConnect() const;

  void setTargetType(odb::dbWireShapeType type) { target_shapes_type_ = type; }

  void makeShapes(const Shape::ShapeTreeMap& other_shapes) override;
  bool refineShapes(Shape::ShapeTreeMap& all_shapes,
                    Shape::ObstructionTreeMap& all_obstructions) override;

  void report() const override;
  Type type() const override { return GridComponent::PadConnect; }

  // disable layer spec checks
  void checkLayerSpecifications() const override {}

  odb::dbITerm* getITerm() const { return iterm_; }

  void getConnectableShapes(Shape::ShapeTreeMap& shapes) const override;

  // cut shapes and remove if connection to ring is not possible
  void cutShapes(const Shape::ObstructionTreeMap& obstructions) override;

  static void unifyConnectionTypes(
      const std::vector<PadDirectConnectionStraps*>& straps);

 private:
  enum class ConnectionType
  {
    None,
    Edge,
    OverPads
  };

  odb::dbITerm* iterm_;
  std::optional<odb::dbWireShapeType> target_shapes_type_;
  std::map<Shape*, Shape*> target_shapes_;
  std::map<Shape*, odb::Rect> target_pin_shape_;
  odb::dbDirection pad_edge_;
  ConnectionType type_ = ConnectionType::None;
  std::vector<odb::dbTechLayer*> layers_;

  std::vector<odb::dbBox*> pins_;

  std::string getName() const;

  // find all the dbBox's to attempt to connect to
  void initialize(ConnectionType type);

  bool isConnectHorizontal() const;

  std::vector<odb::dbBox*> getPinsFacingCore();
  std::vector<odb::dbBox*> getPinsFormingRing();
  std::map<odb::dbTechLayer*, std::vector<odb::dbBox*>> getPinsByLayer() const;

  void makeShapesFacingCore(const Shape::ShapeTreeMap& other_shapes);
  void makeShapesOverPads(const Shape::ShapeTreeMap& other_shapes);

  std::vector<PadDirectConnectionStraps*> getAssociatedStraps() const;
  const std::vector<odb::dbBox*>& getPins() const { return pins_; }

  ShapePtr getClosestShape(const Shape::ShapeTree& search_shapes,
                           const odb::Rect& pin_shape,
                           odb::dbNet* net) const;
  bool snapRectToClosestShape(const ShapePtr& closest_shape,
                              const odb::Rect& pin_shape,
                              odb::Rect& new_shape) const;

  bool strapViaIsObstructed(Shape* shape,
                            const Shape::ShapeTreeMap& other_shapes,
                            const Shape::ObstructionTreeMap& other_obstructions,
                            bool recheck) const;

  void setConnectionType(ConnectionType type);
  ConnectionType getConnectionType() const { return type_; }

  bool refineShape(Shape* shape,
                   const odb::Rect& pin_shape,
                   Shape::ShapeTreeMap& all_shapes,
                   Shape::ObstructionTreeMap& all_obstructions);
  bool isTargetShape(const Shape* shape) const;
};

class RepairChannelStraps : public Straps
{
 public:
  RepairChannelStraps(Grid* grid,
                      Straps* target,
                      odb::dbTechLayer* connect_to,
                      const Shape::ObstructionTreeMap& other_shapes,
                      const std::set<odb::dbNet*>& nets,
                      const odb::Rect& area,
                      const odb::Rect& available_area,
                      const odb::Rect& obs_check_area);

  Type type() const override { return GridComponent::RepairChannel; }

  void report() const override;

  // returns only the nets that need to be repaired.
  std::vector<odb::dbNet*> getNets() const override;

  // cut shapes and remove any segments outside of the area
  void cutShapes(const Shape::ObstructionTreeMap& obstructions) override;

  bool isRepairValid() const { return !invalid_; }

  bool isAtEndOfRepairOptions() const;
  void continueRepairs(const Shape::ObstructionTreeMap& other_shapes);
  bool testBuild(const Shape::ShapeTreeMap& local_shapes,
                 const Shape::ObstructionTreeMap& obstructions);
  bool isEmpty() const;

  bool isAutoInserted() const override { return true; }

  void addNets(const std::set<odb::dbNet*>& nets)
  {
    nets_.insert(nets.begin(), nets.end());
  }
  const odb::Rect& getArea() const { return area_; }

  // static functions to help build repair channels
  // repair unconnected straps by adding repair channel straps
  static void repairGridChannels(Grid* grid,
                                 const Shape::ShapeTreeMap& global_shapes,
                                 Shape::ObstructionTreeMap& obstructions,
                                 bool allow,
                                 PDNRenderer* renderer);

  struct RepairChannelArea
  {
    odb::Rect area;
    odb::Rect available_area;
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
  odb::Rect available_area_;
  odb::Rect obs_check_area_;

  bool invalid_ = false;

  // search for the right width, spacing, and offset to connect to the channel
  void determineParameters(const Shape::ObstructionTreeMap& obstructions);
  bool determineOffset(const Shape::ObstructionTreeMap& obstructions,
                       int extra_offset = 0,
                       int bisect_dist = 0,
                       int level = 0);

  static std::vector<RepairChannelArea> findRepairChannels(
      Grid* grid,
      const Shape::ShapeTree& shapes,
      odb::dbTechLayer* layer);
  static Straps* getTargetStrap(Grid* grid, odb::dbTechLayer* layer);
  static odb::dbTechLayer* getHighestStrapLayer(Grid* grid);

  int getNextWidth() const;
  int getMaxLength() const;
};

}  // namespace pdn
