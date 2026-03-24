// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/fusion/container.hpp"
#include "boost/optional/optional.hpp"
#include "boost/spirit/include/support_unused.hpp"
#include "odb/db.h"
#include "odb/lefin.h"

namespace utl {
class Logger;
}

namespace LefParser {
class lefiLayer;
}  // namespace LefParser

namespace odb {

class lefTechLayerSpacingEolParser
{
 public:
  static void parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerWrongDirSpacingParser
{
 public:
  static void parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerMinStepParser
{
 public:
  bool parse(const std::string&, dbTechLayer*, lefinReader*);

 private:
  odb::dbTechLayerMinStepRule* curRule;
  void createSubRule(odb::dbTechLayer* layer);
  void setMinAdjacentLength1(double length, odb::lefinReader* l);
  void setMinAdjacentLength2(double length, odb::lefinReader* l);
  void minBetweenLengthParser(double length, odb::lefinReader* l);
  void noBetweenEolParser(double width, odb::lefinReader* l);
  void noAdjacentEolParser(double width, odb::lefinReader* l);
  void minStepLengthParser(double length, odb::lefinReader* l);
  void maxEdgesParser(int edges, odb::lefinReader* l);
  void setConvexCorner();
  void setConcaveCorner();
  void setExceptRectangle();
  void setExceptSameCorners();
};

class lefTechLayerCornerSpacingParser
{
 public:
  static bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerSpacingTablePrlParser
{
 public:
  std::vector<int> length_tbl;
  std::vector<int> width_tbl;
  std::vector<std::vector<int>> spacing_tbl;
  std::map<unsigned int, std::pair<int, int>> within_map;
  std::vector<std::tuple<int, int, int>> influence_tbl;
  int curWidthIdx = -1;
  bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerRightWayOnGridOnlyParser
{
 public:
  static bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerRectOnlyParser
{
 public:
  static bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerTypeParser
{
 public:
  static bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerCutClassParser
{
 public:
  static bool parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerCutSpacingParser
{
 public:
  odb::dbTechLayerCutSpacingRule* curRule;
  bool parse(const std::string&,
             dbTechLayer*,
             lefinReader*,
             std::vector<std::pair<odb::dbObject*, std::string>>&);
};

class lefTechLayerCutSpacingTableParser
{
 public:
  odb::dbTechLayerCutSpacingTableDefRule* curRule = nullptr;
  odb::dbTechLayer* layer;
  lefTechLayerCutSpacingTableParser(odb::dbTechLayer* inly) { layer = inly; };
  bool parse(const std::string&,
             lefinReader*,
             std::vector<std::pair<odb::dbObject*, std::string>>&);
};

class lefTechLayerCutEnclosureRuleParser
{
 public:
  lefTechLayerCutEnclosureRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(const std::string&, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerCutEnclosureRule* rule,
              void (odb::dbTechLayerCutEnclosureRule::*func)(int));
  void setCutClass(const std::string& val,
                   odb::dbTechLayerCutEnclosureRule* rule,
                   odb::dbTechLayer* layer);
};
class lefTechLayerEolExtensionRuleParser
{
 public:
  lefTechLayerEolExtensionRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(const std::string&, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerEolExtensionRule* rule,
              void (odb::dbTechLayerEolExtensionRule::*func)(int));
  void addEntry(boost::fusion::vector<double, double>& params,
                odb::dbTechLayerEolExtensionRule* rule);
};
class lefTechLayerEolKeepOutRuleParser
{
 public:
  lefTechLayerEolKeepOutRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  void setExceptWithin(const boost::fusion::vector<double, double>& params,
                       odb::dbTechLayerEolKeepOutRule* rule);
  bool parseSubRule(const std::string&, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerEolKeepOutRule* rule,
              void (odb::dbTechLayerEolKeepOutRule::*func)(int));
  void setClass(const std::string& val,
                odb::dbTechLayerEolKeepOutRule* rule,
                odb::dbTechLayer* layer);
};

class lefTechLayerAreaRuleParser
{
 public:
  lefTechLayerAreaRuleParser(lefinReader*);
  void parse(const std::string&,
             odb::dbTechLayer*,
             std::vector<std::pair<odb::dbObject*, std::string>>&);

 private:
  lefinReader* lefin_;
  bool parseSubRule(
      const std::string&,
      odb::dbTechLayer* layer,
      std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props);
  void setInt(double val,
              odb::dbTechLayerAreaRule* rule,
              void (odb::dbTechLayerAreaRule::*func)(int));
  void setExceptEdgeLengths(const boost::fusion::vector<double, double>& params,
                            odb::dbTechLayerAreaRule* rule);
  void setExceptMinSize(const boost::fusion::vector<double, double>& params,
                        odb::dbTechLayerAreaRule* rule);
  void setExceptStep(const boost::fusion::vector<double, double>& params,
                     odb::dbTechLayerAreaRule* rule);
  void setTrimLayer(
      const std::string& val,
      odb::dbTechLayerAreaRule* rule,
      odb::dbTechLayer* layer,
      std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props);
};

class lefTechLayerPitchRuleParser
{
 public:
  lefTechLayerPitchRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  void setInt(double val,
              odb::dbTechLayer* layer,
              void (odb::dbTechLayer::*func)(int));
  void setPitchXY(boost::fusion::vector<double, double>& params,
                  odb::dbTechLayer* layer);
  lefinReader* lefin_;
};

class lefTechLayerForbiddenSpacingRuleParser
{
 public:
  lefTechLayerForbiddenSpacingRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(const std::string&, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerForbiddenSpacingRule* rule,
              void (odb::dbTechLayerForbiddenSpacingRule::*func)(int));
  void setForbiddenSpacing(const boost::fusion::vector<double, double>& params,
                           odb::dbTechLayerForbiddenSpacingRule* rule);
};

class lefTechLayerTwoWiresForbiddenSpcRuleParser
{
 public:
  lefTechLayerTwoWiresForbiddenSpcRuleParser(lefinReader*);
  void parse(const std::string&, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(const std::string&, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerTwoWiresForbiddenSpcRule* rule,
              void (odb::dbTechLayerTwoWiresForbiddenSpcRule::*func)(int));
  void setForbiddenSpacing(const boost::fusion::vector<double, double>& params,
                           odb::dbTechLayerTwoWiresForbiddenSpcRule* rule);
};

class ArraySpacingParser
{
 public:
  ArraySpacingParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  bool parse(const std::string&);

 private:
  void setCutClass(const std::string& name);
  void setArraySpacing(boost::fusion::vector<int, double>& params);
  void setWithin(boost::fusion::vector<double, double>& params);
  void setCutSpacing(double spacing);
  void setViaWidth(double width);
  dbTechLayer* layer_;
  lefinReader* lefin_;
  dbTechLayerArraySpacingRule* rule_{nullptr};
};

class WidthTableParser
{
 public:
  WidthTableParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  void addWidth(double width);
  bool parseSubRule(const std::string& s);
  dbTechLayer* layer_;
  lefinReader* lefin_;
  dbTechLayerWidthTableRule* rule_{nullptr};
};

class MinCutParser
{
 public:
  MinCutParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  bool parseSubRule(const std::string&);
  void addCutClass(boost::fusion::vector<std::string, int>&);
  void setWidth(double);
  void setWithinCutDist(double);
  void setLength(double);
  void setLengthWithin(double);
  void setArea(double);
  void setAreaWithin(double);
  dbTechLayer* layer_;
  lefinReader* lefin_;
  dbTechLayerMinCutRule* rule_{nullptr};
};

class MetalWidthViaMapParser
{
 public:
  MetalWidthViaMapParser(
      dbTech* tech,
      lefinReader* lefinReader,
      std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
      : tech_(tech), lefin_(lefinReader), incomplete_props_(incomplete_props)
  {
  }
  void parse(const std::string&);

 private:
  bool parseSubRule(const std::string&);
  void addEntry(boost::fusion::vector<std::string,
                                      double,
                                      double,
                                      boost::optional<double>,
                                      boost::optional<double>,
                                      std::string>&);
  void setCutClass();
  void setPGVia();
  dbTech* tech_;
  lefinReader* lefin_;
  bool cut_class_{false};
  std::vector<std::pair<dbObject*, std::string>>& incomplete_props_;
  dbMetalWidthViaMap* via_map{nullptr};
};

class KeepOutZoneParser
{
 public:
  KeepOutZoneParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  bool parseSubRule(const std::string&);
  void setInt(double val, void (odb::dbTechLayerKeepOutZoneRule::*func)(int));
  dbTechLayer* layer_;
  lefinReader* lefin_;
  dbTechLayerKeepOutZoneRule* rule_{nullptr};
};

class MaxSpacingParser
{
 public:
  MaxSpacingParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  void setMaxSpacing(dbTechLayerMaxSpacingRule*, double);
  dbTechLayer* layer_;
  lefinReader* lefin_;
};

class lefTechLayerVoltageSpacing
{
 public:
  lefTechLayerVoltageSpacing(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  dbTechLayer* layer_;
  lefinReader* lefin_;
};

class AntennaGatePlusDiffParser
{
 public:
  AntennaGatePlusDiffParser(LefParser::lefiLayer* layer,
                            lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  LefParser::lefiLayer* layer_;
  lefinReader* lefin_;
};

class MinWidthParser
{
 public:
  MinWidthParser(dbTechLayer* layer, lefinReader* lefinReader)
      : layer_(layer), lefin_(lefinReader)
  {
  }
  void parse(const std::string&);

 private:
  dbTechLayer* layer_;
  lefinReader* lefin_;
};

}  // namespace odb
