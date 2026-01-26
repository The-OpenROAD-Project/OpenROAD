// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shape.h"

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

class Grid;
class VoltageDomain;
class TechLayer;

class GridComponent
{
 public:
  enum Type
  {
    Ring,
    Strap,
    Followpin,
    PadConnect,
    RepairChannel
  };

  explicit GridComponent(Grid* grid);
  virtual ~GridComponent() = default;

  odb::dbBlock* getBlock() const;
  utl::Logger* getLogger() const;

  void setGrid(Grid* grid) { grid_ = grid; }
  Grid* getGrid() const { return grid_; }
  VoltageDomain* getDomain() const;

  bool make(Shape::ShapeTreeMap& shapes,
            Shape::ObstructionTreeMap& obstructions);

  virtual void makeShapes(const Shape::ShapeTreeMap& other_shapes) = 0;
  virtual bool refineShapes(Shape::ShapeTreeMap& all_shapes,
                            Shape::ObstructionTreeMap& all_obstructions)
  {
    return false;
  };

  const Shape::ShapeTreeMap& getShapes() const { return shapes_; }
  void getShapes(Shape::ShapeTreeMap& shapes) const;
  void removeShapes(Shape::ShapeTreeMap& shapes) const;
  void removeShape(Shape* shape);
  void replaceShape(Shape* shape, std::unique_ptr<Shape> replacement);
  void replaceShape(Shape* shape,
                    std::vector<std::unique_ptr<Shape>>& replacements);
  void clearShapes() { shapes_.clear(); }
  int getShapeCount() const;

  virtual void getConnectableShapes(Shape::ShapeTreeMap& shapes) const {}

  // returns all the obstructions in this grid shape
  void getObstructions(Shape::ObstructionTreeMap& obstructions) const;
  void removeObstructions(Shape::ObstructionTreeMap& obstructions) const;

  // cut the shapes according to the obstructions to avoid generating any DRC
  // violations.
  virtual void cutShapes(const Shape::ObstructionTreeMap& obstructions);

  std::map<Shape*, std::vector<odb::dbBox*>> writeToDb(
      const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
      bool add_pins,
      const std::set<odb::dbTechLayer*>& convert_layer_to_pin) const;

  virtual void report() const = 0;
  virtual Type type() const = 0;
  static std::string typeToString(Type type);

  virtual void checkLayerSpecifications() const = 0;

  void setStartWithPower(bool value) { starts_with_power_ = value; }
  bool getStartsWithPower() const { return starts_with_power_; }
  bool getStartsWithGround() const { return !getStartsWithPower(); }

  // returns the order in which to lay out the straps
  virtual std::vector<odb::dbNet*> getNets() const;
  int getNetCount() const;
  void setNets(const std::vector<odb::dbNet*>& nets);

  virtual bool isAutoInserted() const { return false; }

 protected:
  void checkLayerWidth(odb::dbTechLayer* layer,
                       int width,
                       const odb::dbTechLayerDir& direction) const;
  void checkLayerSpacing(odb::dbTechLayer* layer,
                         int width,
                         int spacing,
                         const odb::dbTechLayerDir& direction) const;
  ShapePtr addShape(std::unique_ptr<Shape> shape);

  virtual bool areIntersectionsAllowed() const { return false; }

 private:
  Grid* grid_;
  bool starts_with_power_;
  std::vector<odb::dbNet*> nets_;

  Shape::ShapeTreeMap shapes_;
};

}  // namespace pdn
