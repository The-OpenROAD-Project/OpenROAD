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

#include <map>
#include <set>

#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shape.h"

namespace odb {
class dbBlock;
class dbBox;
class dbNet;
class dbTechLayer;
class dbViaVia;
class dbTechVia;
class dbTechViaGenerateRule;
class dbTechViaLayerRule;
class dbSWire;
class dbVia;
class dbViaParams;
}  // namespace odb

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

  virtual void makeShapes(const ShapeTreeMap& other_shapes) = 0;
  virtual bool refineShapes(ShapeTreeMap& all_shapes,
                            ShapeTreeMap& all_obstructions)
  {
    return false;
  };

  const ShapeTreeMap& getShapes() const { return shapes_; }
  void getShapes(ShapeTreeMap& shapes) const;
  void removeShapes(ShapeTreeMap& shapes) const;
  void removeShape(Shape* shape);
  void replaceShape(Shape* shape, const std::vector<Shape*>& replacements);
  void clearShapes() { shapes_.clear(); }
  int getShapeCount() const;

  virtual void getConnectableShapes(ShapeTreeMap& shapes) const {}

  // returns all the obstructions in this grid shape
  void getObstructions(ShapeTreeMap& obstructions) const;
  void removeObstructions(ShapeTreeMap& obstructions) const;

  // cut the shapes according to the obstructions to avoid generating any DRC
  // violations.
  virtual void cutShapes(const ShapeTreeMap& obstructions);

  void writeToDb(const std::map<odb::dbNet*, odb::dbSWire*>& net_map,
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

 protected:
  void checkLayerWidth(odb::dbTechLayer* layer,
                       int width,
                       const odb::dbTechLayerDir& direction) const;
  void checkLayerSpacing(odb::dbTechLayer* layer,
                         int width,
                         int spacing,
                         const odb::dbTechLayerDir& direction) const;
  ShapePtr addShape(Shape* shape);

 private:
  Grid* grid_;
  bool starts_with_power_;
  std::vector<odb::dbNet*> nets_;

  ShapeTreeMap shapes_;
};

}  // namespace pdn
