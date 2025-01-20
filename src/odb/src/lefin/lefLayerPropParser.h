/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <boost/fusion/container.hpp>
#include <boost/optional/optional.hpp>
#include <boost/spirit/include/support_unused.hpp>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/lefin.h"

namespace utl {
class Logger;
}
namespace odb {
class lefTechLayerSpacingEolParser
{
 public:
  static void parse(const std::string&, dbTechLayer*, lefinReader*);
};

class lefTechLayerWrongDirSpacingParser
{
 public:
  static void parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerMinStepParser
{
 public:
  bool parse(std::string, dbTechLayer*, lefinReader*);

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
  static bool parse(std::string, dbTechLayer*, lefinReader*);
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
  bool parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerRightWayOnGridOnlyParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerRectOnlyParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerTypeParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerCutClassParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefinReader*);
};

class lefTechLayerCutSpacingParser
{
 public:
  odb::dbTechLayerCutSpacingRule* curRule;
  bool parse(std::string,
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
  bool parse(std::string,
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
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerCutEnclosureRule* rule,
              void (odb::dbTechLayerCutEnclosureRule::*func)(int));
  void setCutClass(std::string,
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
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
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
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerEolKeepOutRule* rule,
              void (odb::dbTechLayerEolKeepOutRule::*func)(int));
  void setClass(std::string,
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
      std::string,
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
      std::string val,
      odb::dbTechLayerAreaRule* rule,
      odb::dbTechLayer* layer,
      std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props);
};

class lefTechLayerPitchRuleParser
{
 public:
  lefTechLayerPitchRuleParser(lefinReader*);
  void parse(std::string, odb::dbTechLayer*);

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
  void parse(std::string, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
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
  void parse(std::string, odb::dbTechLayer*);

 private:
  lefinReader* lefin_;
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
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
  bool parse(std::string);

 private:
  void setCutClass(std::string name);
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
  bool parseSubRule(std::string s);
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
  bool parseSubRule(std::string);
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
  bool parseSubRule(std::string);
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
  bool parseSubRule(std::string);
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
  void parse(std::string);

 private:
  void setMaxSpacing(dbTechLayerMaxSpacingRule*, double);
  dbTechLayer* layer_;
  lefinReader* lefin_;
};

}  // namespace odb
