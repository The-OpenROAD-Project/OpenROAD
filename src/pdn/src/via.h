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
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <map>
#include <memory>
#include <set>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shape.h"

namespace utl {
class Logger;
}  // namespace utl

namespace pdn {

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class Connect;
class Shape;
class Via;
class ViaGenerator;

using ViaPtr = std::shared_ptr<Via>;

using uint = odb::uint;

using ViaReport = std::map<std::string, int>;

class Grid;
class TechLayer;

enum class failedViaReason
{
  OBSTRUCTED,
  OVERLAPPING,
  BUILD,
  RIPUP,
  RECHECK,
  OTHER
};

class Enclosure
{
 public:
  Enclosure();
  Enclosure(int x, int y);
  Enclosure(odb::dbTechLayerCutEnclosureRule* rule,
            odb::dbTechLayer* layer,
            const odb::Rect& cut,
            const odb::dbTechLayerDir& direction);
  Enclosure(odb::dbTechViaLayerRule* rule, odb::dbTechLayer* layer);

  int getX() const { return x_; }
  int getY() const { return y_; }

  void setX(int x) { x_ = x; }
  void setY(int y) { y_ = y; }

  bool check(int x, int y) const;

  bool operator<(const Enclosure& other) const;
  bool operator==(const Enclosure& other) const;

  void copy(const Enclosure* other);
  void copy(const Enclosure& other);

  bool isPreferredOver(const Enclosure* other, odb::dbTechLayer* layer) const;
  bool isPreferredOver(const Enclosure* other, bool minimize_x) const;

  void snap(odb::dbTech* tech);

 private:
  int x_;
  int y_;

  bool allow_swap_;

  void swap(odb::dbTechLayer* layer);
};

// Wrapper class to handle building actual ODB DB Vias
class DbVia
{
 public:
  struct ViaLayerShape
  {
    using RectBoxPair = std::pair<odb::Rect, odb::dbSBox*>;
    std::set<RectBoxPair> bottom;
    std::set<RectBoxPair> middle;
    std::set<RectBoxPair> top;
  };

  DbVia();

  virtual ~DbVia() = default;

  virtual ViaLayerShape generate(odb::dbBlock* block,
                                 odb::dbSWire* wire,
                                 odb::dbWireShapeType type,
                                 int x,
                                 int y,
                                 const std::set<odb::dbTechLayer*>& ongrid,
                                 utl::Logger* logger)
      = 0;

  virtual bool requiresPatch() const { return false; }

  odb::Rect adjustToMinArea(odb::dbTechLayer* layer,
                            const odb::Rect& rect) const;

  virtual ViaReport getViaReport() const = 0;

  void setGenerator(const std::shared_ptr<ViaGenerator>& generator)
  {
    generator_ = generator;
  }
  bool hasGenerator() const { return generator_ != nullptr; }
  ViaGenerator* getGenerator() const { return generator_.get(); }

 protected:
  ViaLayerShape getLayerShapes(odb::dbSBox* box) const;
  void combineLayerShapes(const ViaLayerShape& other,
                          ViaLayerShape& shapes) const;

  void addToViaReport(DbVia* via, ViaReport& report) const;

 private:
  std::shared_ptr<ViaGenerator> generator_;
};

// Used as the base class for actual vias like TechVias and GenerateVias
class DbBaseVia : public DbVia
{
 public:
  virtual std::string getName() const = 0;
  virtual odb::Rect getViaRect(bool include_enclosure,
                               bool include_via_shape,
                               bool include_bottom = true,
                               bool include_top = true) const = 0;

  int getCount() const { return count_; }

  ViaReport getViaReport() const override;

 protected:
  void incrementCount() { count_++; }
  void incrementCount(int count) { count_ += count; }

 private:
  int count_ = 0;
};

// Wrapper to handle building dbTechVia as a single via or an array
class DbTechVia : public DbBaseVia
{
 public:
  DbTechVia(odb::dbTechVia* via,
            int rows,
            int row_pitch,
            int cols,
            int col_pitch,
            Enclosure* required_bottom_enc = nullptr,
            Enclosure* required_top_enc = nullptr);

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType type,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  bool requiresPatch() const override { return rows_ > 1 || cols_ > 1; }

  std::string getName() const override;
  odb::Rect getViaRect(bool include_enclosure,
                       bool include_via_shape,
                       bool include_bottom = true,
                       bool include_top = true) const override;

 private:
  odb::dbTechVia* via_;
  odb::dbTechLayer* cut_layer_;
  int rows_;
  int row_pitch_;
  int cols_;
  int col_pitch_;

  odb::Rect via_rect_;
  odb::Rect single_via_rect_;
  odb::Rect enc_bottom_rect_;
  odb::Rect enc_top_rect_;

  odb::Rect required_bottom_rect_;
  odb::Rect required_top_rect_;

  odb::Point via_center_;
  std::set<odb::Point> via_centers_;

  std::string getViaName(const std::set<odb::dbTechLayer*>& ongrid) const;
  bool isArray() const { return rows_ > 1 || cols_ > 1; }
};

// Wrapper to handle building dbTechViaGenerate vias (GENERATE vias) as
// a single via or an array.
class DbGenerateVia : public DbBaseVia
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

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType type,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  std::string getName() const override;
  odb::Rect getViaRect(bool include_enclosure,
                       bool include_via_shape,
                       bool include_bottom = true,
                       bool include_top = true) const override;

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

  std::string getViaName() const;
};

// Wrapper class to build split cut array vias (-split_cut)
class DbSplitCutVia : public DbVia
{
 public:
  DbSplitCutVia(DbBaseVia* via,
                int rows,
                int row_pitch,
                int cols,
                int col_pitch,
                odb::dbBlock* block,
                odb::dbTechLayer* bottom,
                bool snap_bottom,
                odb::dbTechLayer* top,
                bool snap_top);

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType type,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  ViaReport getViaReport() const override;

 private:
  std::unique_ptr<TechLayer> bottom_;
  std::unique_ptr<TechLayer> top_;

  std::unique_ptr<DbBaseVia> via_;
  int rows_;
  int row_pitch_;
  int cols_;
  int col_pitch_;
};

// Wrapper to build via arrays according to ARRAYSPACING rules
class DbArrayVia : public DbVia
{
 public:
  DbArrayVia(DbBaseVia* core_via,
             DbBaseVia* end_of_row,
             DbBaseVia* end_of_column,
             DbBaseVia* end_of_row_column,
             int core_rows,
             int core_columns,
             int array_spacing_x,
             int array_spacing_y);

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType type,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  bool requiresPatch() const override { return true; }

  ViaReport getViaReport() const override;

 private:
  std::unique_ptr<DbBaseVia> core_via_;
  std::unique_ptr<DbBaseVia> end_of_row_;
  std::unique_ptr<DbBaseVia> end_of_column_;
  std::unique_ptr<DbBaseVia> end_of_row_column_;
  int rows_;
  int columns_;

  int array_spacing_x_;
  int array_spacing_y_;

  int array_start_x_ = 0;
  int array_start_y_ = 0;
};

// Wrapper to build multiple level vias as a stack.
class DbGenerateStackedVia : public DbVia
{
 public:
  DbGenerateStackedVia(const std::vector<DbVia*>& vias,
                       odb::dbTechLayer* bottom,
                       odb::dbBlock* block);

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType type,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  ViaReport getViaReport() const override;

 private:
  std::vector<std::unique_ptr<DbVia>> vias_;
  std::vector<std::unique_ptr<TechLayer>> layers_;
};

// Dummy via doesn't build anything but instead generates a warning that a via
// cannot be inserted at the given location.
class DbGenerateDummyVia : public DbVia
{
 public:
  DbGenerateDummyVia(Connect* connect,
                     const odb::Rect& shape,
                     odb::dbTechLayer* bottom,
                     odb::dbTechLayer* top,
                     bool add_report);

  ViaLayerShape generate(odb::dbBlock* block,
                         odb::dbSWire* wire,
                         odb::dbWireShapeType /* type */,
                         int x,
                         int y,
                         const std::set<odb::dbTechLayer*>& ongrid,
                         utl::Logger* logger) override;

  ViaReport getViaReport() const override { return {}; }

 private:
  Connect* connect_;
  bool add_report_;
  const odb::Rect shape_;
  odb::dbTechLayer* bottom_;
  odb::dbTechLayer* top_;
};

// Class to build a generate via, either as a single group or as an array
class ViaGenerator
{
 public:
  struct Constraint
  {
    bool must_fit_x;
    bool must_fit_y;
    bool intersection_only;
  };

  ViaGenerator(utl::Logger* logger,
               const odb::Rect& lower_rect,
               const Constraint& lower_constraint,
               const odb::Rect& upper_rect,
               const Constraint& upper_constraint);
  virtual ~ViaGenerator() = default;

  virtual std::string getName() const = 0;

  virtual odb::dbTechLayer* getBottomLayer() const = 0;
  virtual odb::dbTechLayer* getTopLayer() const = 0;
  virtual odb::dbTechLayer* getCutLayer() const = 0;

  virtual const odb::Rect& getCut() const { return cut_; }
  virtual int getCutArea() const;

  void setCutPitchX(int pitch) { cut_pitch_x_ = pitch; }
  int getCutPitchX() const { return cut_pitch_x_; }
  void setCutPitchY(int pitch) { cut_pitch_y_ = pitch; }
  int getCutPitchY() const { return cut_pitch_y_; }

  void setMaxRows(int rows) { max_rows_ = rows; }
  void setMaxColumns(int columns) { max_cols_ = columns; }

  odb::dbTechLayerCutClassRule* getCutClass() const { return cutclass_; }
  bool hasCutClass() const { return cutclass_ != nullptr; }

  virtual bool isSetupValid(odb::dbTechLayer* lower,
                            odb::dbTechLayer* upper) const;
  virtual bool checkConstraints(bool check_cuts = true,
                                bool check_min_cut = true,
                                bool check_enclosure = true) const;

  // determine the shape of the vias
  bool build(bool bottom_is_internal_layer, bool top_is_internal_layer);
  virtual int getRows() const;
  virtual int getColumns() const;
  virtual int getTotalCuts() const;

  DbVia* generate(odb::dbBlock* block) const;
  virtual DbBaseVia* makeBaseVia(int rows,
                                 int row_pitch,
                                 int cols,
                                 int col_pitch) const = 0;

  const odb::Rect& getLowerRect() const { return lower_rect_; }
  const odb::Rect& getUpperRect() const { return upper_rect_; }
  const odb::Rect& getIntersectionRect() const { return intersection_rect_; }

  void setSplitCutArray(bool split_cuts_bot, bool split_cuts_top);
  bool isSplitCutArray() const { return split_cuts_top_ || split_cuts_bottom_; }
  bool isCutArray() const
  {
    return !isSplitCutArray() && (array_core_x_ != 1 || array_core_y_ != 1);
  }

  Enclosure* getBottomEnclosure() const { return bottom_enclosure_.get(); }
  Enclosure* getTopEnclosure() const { return top_enclosure_.get(); }

  bool isPreferredOver(const ViaGenerator* other) const;

  int getGeneratorWidth(bool bottom) const;
  int getGeneratorHeight(bool bottom) const;

  bool recheckConstraints(const odb::Rect& rect, bool bottom);

 protected:
  int getMaxRows() const { return max_rows_; }
  int getMaxColumns() const { return max_cols_; }

  bool isCutClass(odb::dbTechLayerCutClassRule* cutclass) const;
  void setCut(const odb::Rect& cut);

  int getCuts(int width,
              int cut,
              int bot_enc,
              int top_enc,
              int pitch,
              int max_cuts) const;

  int getCutsWidth(int cuts, int cut_width, int spacing, int enc) const;

  int getViaCoreRows() const { return core_row_; }
  int getViaCoreColumns() const { return core_col_; }
  int getViaLastRows() const { return end_row_; }
  bool hasViaLastRows() const { return end_row_ != 0; }
  int getViaLastColumns() const { return end_col_; }
  bool hasViaLastColumns() const { return end_col_ != 0; }

  int getArraySpacingX() const { return array_spacing_x_; }
  int getArraySpacingY() const { return array_spacing_y_; }

  int getArrayCoresX() const { return array_core_x_; }
  int getArrayCoresY() const { return array_core_y_; }

  utl::Logger* getLogger() const { return logger_; }
  odb::dbTech* getTech() const;

  const Constraint& getLowerConstraint() const { return lower_constraint_; }
  const Constraint& getUpperConstraint() const { return upper_constraint_; }

  int getLowerWidth(bool only_real = true) const;
  int getUpperWidth(bool only_real = true) const;
  int getLowerHeight(bool only_real = true) const;
  int getUpperHeight(bool only_real = true) const;

  void determineCutSpacing();

  virtual void getMinimumEnclosures(std::vector<Enclosure>& bottom,
                                    std::vector<Enclosure>& top,
                                    bool rules_only) const;

 private:
  utl::Logger* logger_;

  odb::Rect lower_rect_;
  odb::Rect upper_rect_;
  odb::Rect intersection_rect_;

  Constraint lower_constraint_;
  Constraint upper_constraint_;

  odb::Rect cut_;

  odb::dbTechLayerCutClassRule* cutclass_ = nullptr;

  int cut_pitch_x_ = 0;
  int cut_pitch_y_ = 0;

  int max_rows_ = 0;
  int max_cols_ = 0;

  int core_row_ = 0;
  int core_col_ = 0;
  int end_row_ = 0;
  int end_col_ = 0;

  bool split_cuts_bottom_ = false;
  bool split_cuts_top_ = false;

  int array_spacing_x_ = 0;
  int array_spacing_y_ = 0;
  int array_core_x_ = 1;
  int array_core_y_ = 1;

  std::unique_ptr<Enclosure> bottom_enclosure_;
  std::unique_ptr<Enclosure> top_enclosure_;

  void determineCutClass();
  bool checkMinCuts() const;
  bool checkMinCuts(odb::dbTechLayer* layer, int width) const;
  bool appliesToLayers(odb::dbTechLayer* lower, odb::dbTechLayer* upper) const;

  bool checkMinEnclosure() const;

  std::vector<odb::dbTechLayerCutEnclosureRule*> getCutMinimumEnclosureRules(
      int width,
      bool above) const;

  void determineRowsAndColumns(bool use_bottom_min_enclosure,
                               bool use_top_min_enclosure,
                               const Enclosure& bottom_min_enclosure,
                               const Enclosure& top_min_enclosure);

  odb::dbTechLayerDir getRectDirection(const odb::Rect& rect) const;

  int getRectSize(const odb::Rect& rect, bool min, bool only_real) const;

  bool updateCutSpacing(int rows, int cols);
};

// Class to build a generate via, either as a single group or as an array
class GenerateViaGenerator : public ViaGenerator
{
 public:
  GenerateViaGenerator(utl::Logger* logger,
                       odb::dbTechViaGenerateRule* rule,
                       const odb::Rect& lower_rect,
                       const Constraint& lower_constraint,
                       const odb::Rect& upper_rect,
                       const Constraint& upper_constraint);

  std::string getName() const override;
  std::string getRuleName() const;

  odb::dbTechLayer* getBottomLayer() const override;
  odb::dbTechViaLayerRule* getBottomLayerRule() const;
  odb::dbTechLayer* getTopLayer() const override;
  odb::dbTechViaLayerRule* getTopLayerRule() const;
  odb::dbTechLayer* getCutLayer() const override;
  odb::dbTechViaLayerRule* getCutLayerRule() const;

  bool isSetupValid(odb::dbTechLayer* lower,
                    odb::dbTechLayer* upper) const override;

  DbBaseVia* makeBaseVia(int rows,
                         int row_pitch,
                         int cols,
                         int col_pitch) const override;

 protected:
  void getMinimumEnclosures(std::vector<Enclosure>& bottom,
                            std::vector<Enclosure>& top,
                            bool rules_only) const override;

 private:
  odb::dbTechViaGenerateRule* rule_;

  std::array<odb::uint, 3> layers_;

  bool isLayerValidForWidth(odb::dbTechViaLayerRule*, int width) const;
  bool getLayerEnclosureRule(odb::dbTechViaLayerRule* rule,
                             int& dx,
                             int& dy) const;
  bool isBottomValidForWidth(int width) const;
  bool isTopValidForWidth(int width) const;
};

// Class to build a generate via, either as a single group or as an array
class TechViaGenerator : public ViaGenerator
{
 public:
  TechViaGenerator(utl::Logger* logger,
                   odb::dbTechVia* via,
                   const odb::Rect& lower_rect,
                   const Constraint& lower_constraint,
                   const odb::Rect& upper_rect,
                   const Constraint& upper_constraint);

  std::string getName() const override;

  odb::dbTechLayer* getBottomLayer() const override { return bottom_; }
  odb::dbTechLayer* getTopLayer() const override { return top_; }
  odb::dbTechLayer* getCutLayer() const override { return cut_; }

  const odb::Rect& getCut() const override;
  int getCutArea() const override;

  int getTotalCuts() const override;

  bool isSetupValid(odb::dbTechLayer* lower,
                    odb::dbTechLayer* upper) const override;

  odb::dbTechVia* getVia() const { return via_; }

  DbBaseVia* makeBaseVia(int rows,
                         int row_pitch,
                         int cols,
                         int col_pitch) const override;

  static std::set<odb::Rect> getViaObstructionRects(utl::Logger* logger,
                                                    odb::dbTechVia* via,
                                                    int x,
                                                    int y);

 protected:
  void getMinimumEnclosures(std::vector<Enclosure>& bottom,
                            std::vector<Enclosure>& top,
                            bool rules_only) const override;

 private:
  odb::dbTechVia* via_;

  int cuts_ = 0;
  odb::Rect cut_outline_;

  odb::dbTechLayer* bottom_;
  odb::dbTechLayer* cut_ = nullptr;
  odb::dbTechLayer* top_;

  bool fitsShapes() const;
  bool mostlyContains(const odb::Rect& full_shape,
                      const odb::Rect& intersection,
                      const odb::Rect& small_shape,
                      const Constraint& constraint) const;
};

class Via
{
 public:
  struct ViaIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(const ViaPtr& via) const { return via->getArea(); }
  };
  using ViaTree = bgi::rtree<ViaPtr, bgi::quadratic<16>, ViaIndexableGetter>;

  Via(Connect* connect,
      odb::dbNet* net,
      const odb::Rect& area,
      const ShapePtr& lower,
      const ShapePtr& upper);

  odb::dbNet* getNet() const { return net_; }
  const odb::Rect& getArea() const { return area_; }
  void setLowerShape(const ShapePtr& shape) { lower_ = shape; }
  const ShapePtr& getLowerShape() const { return lower_; }
  void setUpperShape(const ShapePtr& shape) { upper_ = shape; }
  const ShapePtr& getUpperShape() const { return upper_; }
  odb::dbTechLayer* getLowerLayer() const;
  odb::dbTechLayer* getUpperLayer() const;

  void removeShape(Shape* shape);

  bool isValid() const;

  bool containsIntermediateLayer(odb::dbTechLayer* layer) const;
  bool overlaps(const ViaPtr& via) const;
  bool startsBelow(const ViaPtr& via) const;

  Connect* getConnect() const { return connect_; }

  void writeToDb(odb::dbSWire* wire,
                 odb::dbBlock* block,
                 const Shape::ObstructionTreeMap& obstructions);

  Grid* getGrid() const;

  std::string getDisplayText() const;

  Via* copy() const;

  void markFailed(failedViaReason reason);
  bool isFailed() const { return failed_; }

  static ViaTree convertVectorToTree(std::vector<ViaPtr>& vec);

 private:
  odb::dbNet* net_;
  odb::Rect area_;
  ShapePtr lower_;
  ShapePtr upper_;

  Connect* connect_;

  bool failed_ = false;

  utl::Logger* getLogger() const;
};

}  // namespace pdn
