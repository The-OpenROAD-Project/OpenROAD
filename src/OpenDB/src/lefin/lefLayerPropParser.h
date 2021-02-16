#pragma once

#include <map>
#include <string>
#include <vector>

#include "db.h"
#include "lefin.h"

namespace odb {
class lefTechLayerSpacingEolParser
{
 public:
  static bool parse(std::string, dbTechLayer*, lefin*);
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
  std::vector<int>                            length_tbl;
  std::vector<int>                            width_tbl;
  std::vector<std::vector<int>>               spacing_tbl;
  std::map<unsigned int, std::pair<int, int>> within_map;
  std::vector<std::tuple<int, int, int>>      influence_tbl;
  int                                         curWidthIdx = -1;
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
  bool                            parse(std::string, dbTechLayer*, lefin*);
};

class lefTechLayerCutSpacingTableParser
{
 public:
  odb::dbTechLayerCutSpacingTableDefRule* curRule;
  odb::dbTechLayer*                       layer;
  lefTechLayerCutSpacingTableParser(odb::dbTechLayer* inly) { layer = inly; };
  bool parse(std::string, lefin*);
};

}  // namespace odb
