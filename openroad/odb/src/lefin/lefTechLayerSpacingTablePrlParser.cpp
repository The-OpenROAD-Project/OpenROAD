// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>
#include <tuple>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerSpacingTablePrl {
namespace {

void addInfluence(boost::fusion::vector<double, double, double>& params,
                  odb::lefTechLayerSpacingTablePrlParser* parser,
                  odb::lefinReader* lefinReader)
{
  auto width = lefinReader->dbdist(at_c<0>(params));
  auto distance = lefinReader->dbdist(at_c<1>(params));
  auto spacing = lefinReader->dbdist(at_c<2>(params));
  parser->influence_tbl.emplace_back(width, distance, spacing);
}
void addLength(double val,
               odb::lefTechLayerSpacingTablePrlParser* parser,
               odb::lefinReader* lefinReader)
{
  parser->length_tbl.push_back(lefinReader->dbdist(val));
}
void addWidth(double val,
              odb::lefTechLayerSpacingTablePrlParser* parser,
              odb::lefinReader* lefinReader)
{
  parser->width_tbl.push_back(lefinReader->dbdist(val));
  parser->curWidthIdx++;
  parser->spacing_tbl.emplace_back();
}
void addExcluded(boost::fusion::vector<double, double>& params,
                 odb::lefTechLayerSpacingTablePrlParser* parser,
                 odb::lefinReader* lefinReader)
{
  auto low = lefinReader->dbdist(at_c<0>(params));
  auto high = lefinReader->dbdist(at_c<1>(params));
  parser->within_map[parser->curWidthIdx] = {low, high};
}
void addSpacing(double val,
                odb::lefTechLayerSpacingTablePrlParser* parser,
                odb::lefinReader* lefinReader)
{
  parser->spacing_tbl[parser->curWidthIdx].push_back(lefinReader->dbdist(val));
}
void setEolWidth(double val,
                 odb::lefTechLayerSpacingTablePrlParser* parser,
                 odb::lefinReader* lefinReader)
{
  parser->rules.back()->setEolWidth(lefinReader->dbdist(val));
}
void setWrongDirection(odb::lefTechLayerSpacingTablePrlParser* parser)
{
  parser->rules.back()->setWrongDirection(true);
}
void setSameMask(odb::lefTechLayerSpacingTablePrlParser* parser)
{
  parser->rules.back()->setSameMask(true);
}
void setExceptEol(odb::lefTechLayerSpacingTablePrlParser* parser)
{
  parser->rules.back()->setExceeptEol(true);
}
void createRule(odb::dbTechLayer* layer,
                odb::lefTechLayerSpacingTablePrlParser* parser)
{
  parser->rules.push_back(odb::dbTechLayerSpacingTablePrlRule::create(layer));
  parser->length_tbl.clear();
  parser->width_tbl.clear();
  parser->spacing_tbl.clear();
  parser->within_map.clear();
  parser->influence_tbl.clear();
  parser->curWidthIdx = -1;
}
void finishRule(odb::lefTechLayerSpacingTablePrlParser* parser)
{
  bool valid = parser->spacing_tbl.size() == parser->width_tbl.size();
  for (const auto& spacing : parser->spacing_tbl) {
    if (spacing.size() != parser->length_tbl.size()) {
      valid = false;
    }
  }
  if (!valid) {
    parser->tables_valid = false;
    return;
  }
  odb::dbTechLayerSpacingTablePrlRule* rule = parser->rules.back();
  rule->setTable(parser->width_tbl,
                 parser->length_tbl,
                 parser->spacing_tbl,
                 parser->within_map);
  rule->setSpacingTableInfluence(parser->influence_tbl);
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader,
           odb::lefTechLayerSpacingTablePrlParser* parser)
{
  qi::rule<std::string::const_iterator, space_type> SPACINGTABLE
      = (lit("SPACINGTABLE") >> lit("INFLUENCE")
         >> +(lit("WIDTH") >> double_ >> lit("WITHIN") >> double_
              >> lit("SPACING")
              >> double_)[boost::bind(&addInfluence, _1, parser, lefinReader)]
         >> lit(";"));

  qi::rule<std::string::const_iterator, space_type> spacingTableRule
      = ((lit("SPACINGTABLE")
          >> lit("PARALLELRUNLENGTH"))[boost::bind(&createRule, layer, parser)]
         >> -lit("WRONGDIRECTION")[boost::bind(&setWrongDirection, parser)]
         >> -lit("SAMEMASK")[boost::bind(&setSameMask, parser)]
         >> -(lit("EXCEPTEOL")[boost::bind(&setExceptEol, parser)]
              >> double_[boost::bind(&setEolWidth, _1, parser, lefinReader)])
         >> +double_[boost::bind(&addLength, _1, parser, lefinReader)] >> +(
             lit("WIDTH")
             >> double_[boost::bind(&addWidth, _1, parser, lefinReader)] >> -(
                 lit("EXCEPTWITHIN") >> double_
                 >> double_)[boost::bind(&addExcluded, _1, parser, lefinReader)]
             >> +double_[boost::bind(&addSpacing, _1, parser, lefinReader)])
         >> -SPACINGTABLE >> lit(";")[boost::bind(&finishRule, parser)]);

  // A single property may hold several SPACINGTABLE statements, each
  // becoming its own dbTechLayerSpacingTablePrlRule.
  const bool valid = qi::phrase_parse(first, last, +spacingTableRule, space)
                     && first == last && parser->tables_valid;

  if (!valid) {  // fail if we did not get a full match
    for (odb::dbTechLayerSpacingTablePrlRule* rule : parser->rules) {
      odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
    }
  }
  return valid;
}
}  // namespace
}  // namespace odb::lefTechLayerSpacingTablePrl

namespace odb {

bool lefTechLayerSpacingTablePrlParser::parse(const std::string& s,
                                              dbTechLayer* layer,
                                              odb::lefinReader* l)
{
  return lefTechLayerSpacingTablePrl::parse(s.begin(), s.end(), layer, l, this);
}

}  // namespace odb
