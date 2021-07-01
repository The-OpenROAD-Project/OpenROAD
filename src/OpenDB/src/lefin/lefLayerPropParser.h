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
#include <map>
#include <string>
#include <vector>

#include "db.h"
#include "lefin.h"
namespace utl {
class Logger;
}
namespace odb {
class lefTechLayerSpacingEolParser
{
 public:
  static void parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerMinStepParser
{
 public:
  bool parse(std::string, dbTechLayer*, lefin*);

 private:
  odb::dbTechLayerMinStepRule* curRule;
  void createSubRule(odb::dbTechLayer* layer);
  void setMinAdjacentLength1(double length, odb::lefin* l);
  void setMinAdjacentLength2(double length, odb::lefin* l);
  void minBetweenLngthParser(double length, odb::lefin* l);
  void noBetweenEolParser(double width, odb::lefin* l);
  void minStepLengthParser(double length, odb::lefin* l);
  void maxEdgesParser(int edges, odb::lefin* l);
  void setConvexCorner();
  void setExceptSameCorners();
};

class lefTechLayerCornerSpacingParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
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
  bool parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerRightWayOnGridOnlyParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerRectOnlyParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerTypeParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerCutClassParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerCutSpacingParser
{
 public:
  odb::dbTechLayerCutSpacingRule* curRule;
  bool parse(std::string,
             dbTechLayer*,
             lefin*,
             std::vector<std::pair<odb::dbObject*, std::string>>&);
};

class lefTechLayerCutSpacingTableParser
{
 public:
  odb::dbTechLayerCutSpacingTableDefRule* curRule;
  odb::dbTechLayer* layer;
  lefTechLayerCutSpacingTableParser(odb::dbTechLayer* inly) { layer = inly; };
  bool parse(std::string,
             lefin*,
             std::vector<std::pair<odb::dbObject*, std::string>>&);
};

class lefTechLayerCutEnclosureRuleParser
{
 public:
  lefTechLayerCutEnclosureRuleParser(lefin*);
  void parse(std::string, odb::dbTechLayer*);

 private:
  lefin* lefin_;
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
  lefTechLayerEolExtensionRuleParser(lefin*);
  void parse(std::string, odb::dbTechLayer*);

 private:
  lefin* lefin_;
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
  lefTechLayerEolKeepOutRuleParser(lefin*);
  void parse(std::string, odb::dbTechLayer*);

 private:
  lefin* lefin_;
  bool parseSubRule(std::string, odb::dbTechLayer* layer);
  void setInt(double val,
              odb::dbTechLayerEolKeepOutRule* rule,
              void (odb::dbTechLayerEolKeepOutRule::*func)(int));
  void setClass(std::string,
                odb::dbTechLayerEolKeepOutRule* rule,
                odb::dbTechLayer* layer);
};

}  // namespace odb
