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
  odb::dbTechLayerMinStepRule* curRule;
  bool                         parse(std::string, dbTechLayer*, lefin*);
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
