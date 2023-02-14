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

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <map>
#include <memory>
#include <vector>

#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "via.h"

namespace odb {
class dbBlock;
class dbBox;
class dbNet;
class dbTechLayer;
class dbViaVia;
class dbTechVia;
class dbTechViaGenerateRule;
class dbTechViaLayerRule;
class dbRow;
class dbSWire;
class dbVia;
class dbViaParams;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class Shape;

using Point = bg::model::d2::point_xy<int, bg::cs::cartesian>;
using Box = bg::model::box<Point>;
using ShapePtr = std::shared_ptr<Shape>;
using ViaPtr = std::shared_ptr<Via>;
using ShapeValue = std::pair<Box, ShapePtr>;
using ViaValue = std::pair<Box, ViaPtr>;
using ShapeTree = bgi::rtree<ShapeValue, bgi::quadratic<16>>;
using ViaTree = bgi::rtree<ViaValue, bgi::quadratic<16>>;
using ShapeTreeMap = std::map<odb::dbTechLayer*, ShapeTree>;

class Grid;
class GridComponent;
class VoltageDomain;

// Basic class that contains a single shape for the power grid, keeping track of
// via connections, iterm connections, and bterm connections.
class Shape
{
 public:
  enum ShapeType
  {
    SHAPE,
    GRID_OBS,
    BLOCK_OBS,
    OBS,
    FIXED
  };
  Shape(odb::dbTechLayer* layer,
        odb::dbNet* net,
        const odb::Rect& rect,
        odb::dbWireShapeType type = odb::dbWireShapeType::NONE);
  Shape(odb::dbTechLayer* layer, const odb::Rect& rect, ShapeType shape_type);
  virtual ~Shape();

  odb::dbTechLayer* getLayer() const { return layer_; }
  odb::dbNet* getNet() const { return net_; }
  void setNet(odb::dbNet* net) { net_ = net; }
  void setRect(const odb::Rect& rect) { rect_ = rect; }
  const odb::Rect& getRect() const { return rect_; }
  const Box getRectBox() const;
  odb::dbWireShapeType getType() const { return type_; }

  utl::Logger* getLogger() const;

  ShapeType shapeType() const { return shape_type_; }
  void setShapeType(ShapeType type) { shape_type_ = type; }

  int getLength() const { return rect_.maxDXDY(); }
  int getWidth() const { return rect_.minDXDY(); }
  // true if the shape is aligned against the preferred direction of the layer
  bool isWrongWay() const;

  // check if shape is valid for the given layer
  bool isValid() const;

  const odb::Rect& getObstruction() const { return obs_; }
  const Box getObstructionBox() const;
  // generates the obstruction box needed to avoid DRC violations with
  // surrounding shapes
  void generateObstruction();
  void setObstruction(const odb::Rect& rect) { obs_ = rect; }

  bool isHorizontal() const { return rect_.dx() > rect_.dy(); }
  bool isSquare() const { return rect_.dx() == rect_.dy(); }
  bool isVertical() const { return rect_.dx() < rect_.dy(); }

  // true if shape can be removed by trimming
  virtual bool isRemovable() const;
  // true if shape can be modified (cut or shortened) by trimming
  virtual bool isModifiable() const;

  void clearVias() { vias_.clear(); }
  void addVia(const ViaPtr& via) { vias_.push_back(via); }
  const std::vector<ViaPtr>& getVias() const { return vias_; }
  void removeVia(const ViaPtr& via);

  void addITermConnection(const odb::Rect& iterm)
  {
    iterm_connections_.insert(iterm);
  }
  void removeITermConnection(const odb::Rect& iterm)
  {
    iterm_connections_.erase(iterm);
  }
  const std::set<odb::Rect>& getItermConnections() const
  {
    return iterm_connections_;
  }
  void addBTermConnection(const odb::Rect& bterm)
  {
    bterm_connections_.insert(bterm);
  }
  void removeBTermConnection(const odb::Rect& bterm)
  {
    bterm_connections_.erase(bterm);
  }
  const std::set<odb::Rect>& getBtermConnections() const
  {
    return bterm_connections_;
  }
  // after shape is modified, remove any term connections that are no longer
  // connected
  virtual void updateTermConnections();
  bool hasTermConnections() const;

  // returns the smallest shape possible when attempting to trim
  virtual const odb::Rect getMinimumRect() const;
  int getNumberOfConnections() const;
  int getNumberOfConnectionsBelow() const;
  int getNumberOfConnectionsAbove() const;

  Shape* extendTo(const odb::Rect& rect, const ShapeTree& obstructions) const;

  virtual bool cut(const ShapeTree& obstructions,
                   std::vector<Shape*>& replacements) const;

  // return a copy of the shape
  virtual Shape* copy() const;
  // merge this shape with another
  virtual void merge(Shape* shape);

  void setGridComponent(GridComponent* component)
  {
    grid_component_ = component;
  }
  GridComponent* getGridComponent() const { return grid_component_; }

  // returns the text used by the renderer to identify the shape
  const std::string getDisplayText() const;

  const std::string getReportText() const;
  static const std::string getRectText(const odb::Rect& rect,
                                       double dbu_to_micron);

  void writeToDb(odb::dbSWire* swire,
                 bool add_pins,
                 bool make_shape_rect) const;
  // copy existing shapes into the map
  static void populateMapFromDb(odb::dbNet* net, ShapeTreeMap& map);

  static const Box rectToBox(const odb::Rect& rect);

  bool allowsNonPreferredDirectionChange() const
  {
    return allow_non_preferred_change_;
  }
  virtual void setAllowsNonPreferredDirectionChange()
  {
    allow_non_preferred_change_ = true;
  }

 protected:
  bool cut(const ShapeTree& obstructions,
           std::vector<Shape*>& replacements,
           const std::function<bool(const ShapeValue&)>& obs_filter) const;

 private:
  odb::dbTechLayer* layer_;
  odb::dbNet* net_;
  odb::Rect rect_;
  odb::dbWireShapeType type_;
  ShapeType shape_type_;
  bool allow_non_preferred_change_;

  odb::Rect obs_;

  GridComponent* grid_component_;

  std::vector<ViaPtr> vias_;
  std::set<odb::Rect> iterm_connections_;
  std::set<odb::Rect> bterm_connections_;

  // add rect as bterm to database
  void addBPinToDb(const odb::Rect& rect) const;

  void updateIBTermConnections(std::set<odb::Rect>& terms);
};

class FollowPinShape : public Shape
{
 public:
  FollowPinShape(odb::dbTechLayer* layer,
                 odb::dbNet* net,
                 const odb::Rect& rect);
  ~FollowPinShape() {}

  void addRow(odb::dbRow* row) { rows_.insert(row); }

  virtual const odb::Rect getMinimumRect() const override;
  virtual Shape* copy() const override;
  virtual void merge(Shape* shape) override;
  virtual void updateTermConnections() override;

  // followpins cannot be removed
  virtual bool isRemovable() const override { return false; }

  virtual void setAllowsNonPreferredDirectionChange() override {}

  virtual bool cut(const ShapeTree& obstructions,
                   std::vector<Shape*>& replacements) const override;

 private:
  std::set<odb::dbRow*> rows_;
};

}  // namespace pdn
