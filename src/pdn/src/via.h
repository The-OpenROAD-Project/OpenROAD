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
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <map>
#include <memory>

#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace odb {
class dbBlock;
class dbNet;
class dbTechLayer;
class dbTechLayerCutClassRule;
class dbViaVia;
class dbTechVia;
class dbTechViaGenerateRule;
class dbTechViaLayerRule;
class dbSBox;
class dbSWire;
class dbVia;
class dbViaParams;
}  // namespace odb

namespace pdn {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class Connect;
class Shape;
class Via;

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
class GridShape;

// Wrapper class to handle building actual DB Vias
class DbVia
{
 public:
  struct ViaLayerShape
  {
    odb::Rect bottom;
    odb::Rect top;
  };

  DbVia() = default;
  virtual ~DbVia() {}

  virtual ViaLayerShape generate(odb::dbBlock* block,
                                 odb::dbSWire* wire,
                                 odb::dbWireShapeType type,
                                 int x,
                                 int y) const = 0;

  virtual bool isArray() const { return false; }

 protected:
  ViaLayerShape getLayerShapes(odb::dbSBox* box) const;
};

class DbTechVia : public DbVia
{
 public:
  DbTechVia(odb::dbTechVia* via);
  virtual ~DbTechVia() {}

  virtual ViaLayerShape generate(odb::dbBlock* block,
                                 odb::dbSWire* wire,
                                 odb::dbWireShapeType type,
                                 int x,
                                 int y) const override;

 private:
  odb::dbTechVia* via_;
};

class DbGenerateVia : public DbVia
{
 public:
  DbGenerateVia(const odb::Rect& rect,
                odb::dbTechViaGenerateRule* rule,
                int rows,
                int columns,
                int cut_pitch_x,
                int cut_pitch_y,
                int bottom_enclosure_x,
                int bottom_enclosure_y,
                int top_enclosure_x,
                int top_enclosure_y,
                odb::dbTechLayer* bottom,
                odb::dbTechLayer* cut,
                odb::dbTechLayer* top);
  virtual ~DbGenerateVia() {}

  virtual ViaLayerShape generate(odb::dbBlock* block,
                                 odb::dbSWire* wire,
                                 odb::dbWireShapeType type,
                                 int x,
                                 int y) const override;

  const odb::Rect getViaRect(bool include_enclosure = true) const;

 private:
  odb::Rect rect_;
  odb::Rect cut_rect_;

  odb::dbTechViaGenerateRule* rule_;
  int rows_;
  int columns_;

  int cut_pitch_x_;
  int cut_pitch_y_;

  int bottom_enclosure_x_;
  int bottom_enclosure_y_;
  int top_enclosure_x_;
  int top_enclosure_y_;

  odb::dbTechLayer* bottom_;
  odb::dbTechLayer* cut_;
  odb::dbTechLayer* top_;

  const std::string getName() const;
};

class DbGenerateArrayVia : public DbVia
{
 public:
  DbGenerateArrayVia(DbGenerateVia* core_via,
                     DbGenerateVia* end_of_row,
                     DbGenerateVia* end_of_column,
                     DbGenerateVia* end_of_row_column,
                     int core_rows,
                     int core_cols,
                     int array_spacing_x,
                     int array_spacing_y);
  virtual ~DbGenerateArrayVia() {}

  virtual ViaLayerShape generate(odb::dbBlock* block,
                                 odb::dbSWire* wire,
                                 odb::dbWireShapeType type,
                                 int x,
                                 int y) const override;

  virtual bool isArray() const override { return true; }

 private:
  std::unique_ptr<DbGenerateVia> core_via_;
  std::unique_ptr<DbGenerateVia> end_of_row_;
  std::unique_ptr<DbGenerateVia> end_of_column_;
  std::unique_ptr<DbGenerateVia> end_of_row_column_;
  int rows_;
  int columns_;

  int array_spacing_x_;
  int array_spacing_y_;

  int array_start_x_;
  int array_start_y_;
};

// Class to build a generate via, either as a single group or as an array
class GenerateVia
{
 public:
  GenerateVia(odb::dbTechViaGenerateRule* rule,
              const odb::Rect& lower_rect,
              const odb::Rect& upper_rect);

  const std::string getName() const;
  const std::string getRuleName() const;

  odb::dbTechLayer* getBottomLayer() const;
  odb::dbTechViaLayerRule* getBottomLayerRule() const;
  odb::dbTechLayer* getTopLayer() const;
  odb::dbTechViaLayerRule* getTopLayerRule() const;
  odb::dbTechLayer* getCutLayer() const;
  odb::dbTechViaLayerRule* getCutLayerRule() const;

  bool isBottomValidForWidth(int width) const;
  bool isTopValidForWidth(int width) const;

  const odb::Rect getCut() const;
  int getCutArea() const;

  void setCutPitchX(int pitch) { cut_pitch_x_ = pitch; }
  int getCutPitchX() const { return cut_pitch_x_; }
  void setCutPitchY(int pitch) { cut_pitch_y_ = pitch; }
  int getCutPitchY() const { return cut_pitch_y_; }

  odb::dbTechLayerCutClassRule* getCutClass() const { return cutclass_; }

  // determine the shape of the vias
  void determineRowsAndColumns(bool use_bottom_min_enclosure,
                               bool use_top_min_enclosure);
  int getTotalCuts() const;

  bool checkMinCuts() const;
  bool checkMinEnclosure() const;

  bool isCutArray() const { return array_core_x_ != 1 || array_core_y_ != 1; }

  DbVia* generate(odb::dbBlock* block) const;

 private:
  odb::dbTechViaGenerateRule* rule_;
  odb::Rect lower_rect_;
  odb::Rect upper_rect_;
  odb::Rect intersection_rect_;

  odb::dbTechLayerCutClassRule* cutclass_;

  int core_row_;
  int core_col_;
  int end_row_;
  int end_col_;

  int array_spacing_x_;
  int array_spacing_y_;
  int array_core_x_;
  int array_core_y_;

  int cut_pitch_x_;
  int cut_pitch_y_;

  int bottom_x_enclosure_;
  int bottom_y_enclosure_;
  int top_x_enclosure_;
  int top_y_enclosure_;

  std::array<uint, 3> layers_;

  bool isLayerValidForWidth(odb::dbTechViaLayerRule*, int width) const;
  void getLayerEnclosureRule(odb::dbTechViaLayerRule* rule,
                             int& dx,
                             int& dy) const;
  void getMinimumEnclosure(odb::dbTechLayer* layer,
                           int width,
                           int& dx,
                           int& dy) const;

  struct ArraySpacing
  {
    int width;
    bool longarray;
    int cut_spacing;
    int cuts;
    int array_spacing;
  };
  std::vector<ArraySpacing> getArraySpacing() const;

  int getRows() const;
  int getColumns() const;

  void getCuts(int width,
               int cut,
               int bot_enc,
               int top_enc,
               int pitch,
               int& cuts) const;
  int getCutsWidth(int cuts, int cut_width, int spacing, int enc) const;

  void determineCutClass();
};

class Via
{
 public:
  Via(Connect* connect,
      odb::dbNet* net,
      const odb::Rect& area,
      const ShapePtr& lower,
      const ShapePtr& upper);

  odb::dbNet* getNet() const { return net_; }
  const odb::Rect& getArea() const { return area_; }
  const Box getBox() const;
  void setLowerShape(ShapePtr shape) { lower_ = shape; }
  const ShapePtr& getLowerShape() const { return lower_; }
  void setUpperShape(ShapePtr shape) { upper_ = shape; }
  const ShapePtr& getUpperShape() const { return upper_; }
  odb::dbTechLayer* getLowerLayer() const;
  odb::dbTechLayer* getUpperLayer() const;

  void removeShape(Shape* shape);

  bool isValid() const;

  bool containsIntermediateLayer(odb::dbTechLayer* layer) const;
  bool overlaps(const ViaPtr& via) const;
  bool startsBelow(const ViaPtr& via) const;

  Connect* getConnect() const { return connect_; }

  void writeToDb(odb::dbSWire* wire, odb::dbBlock* block) const;

  Grid* getGrid() const;

  const std::string getDisplayText() const;

  Via* copy() const;

 private:
  odb::dbNet* net_;
  odb::Rect area_;
  ShapePtr lower_;
  ShapePtr upper_;

  Connect* connect_;
};

}  // namespace pdn
