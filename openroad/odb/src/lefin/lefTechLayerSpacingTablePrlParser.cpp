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
                 odb::dbTechLayerSpacingTablePrlRule* rule,
                 odb::lefinReader* lefinReader)
{
  rule->setEolWidth(lefinReader->dbdist(val));
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader,
           odb::lefTechLayerSpacingTablePrlParser* parser)
{
  odb::dbTechLayerSpacingTablePrlRule* rule
      = odb::dbTechLayerSpacingTablePrlRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> SPACINGTABLE
      = (lit("SPACINGTABLE") >> lit("INFLUENCE")
         >> +(lit("WIDTH") >> double_ >> lit("WITHIN") >> double_
              >> lit("SPACING")
              >> double_)[boost::bind(&addInfluence, _1, parser, lefinReader)]
         >> lit(";"));

  qi::rule<std::string::const_iterator, space_type> spacingTableRule
      = (lit("SPACINGTABLE") >> lit("PARALLELRUNLENGTH")
         >> -lit("WRONGDIRECTION")[boost::bind(
             &odb::dbTechLayerSpacingTablePrlRule::setWrongDirection,
             rule,
             true)]
         >> -lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerSpacingTablePrlRule::setSameMask, rule, true)]
         >> -(lit("EXCEPTEOL")[boost::bind(
                  &odb::dbTechLayerSpacingTablePrlRule::setExceeptEol,
                  rule,
                  true)]
              >> double_[boost::bind(&setEolWidth, _1, rule, lefinReader)])
         >> +double_[boost::bind(&addLength, _1, parser, lefinReader)] >> +(
             lit("WIDTH")
             >> double_[boost::bind(&addWidth, _1, parser, lefinReader)] >> -(
                 lit("EXCEPTWITHIN") >> double_
                 >> double_)[boost::bind(&addExcluded, _1, parser, lefinReader)]
             >> +double_[boost::bind(&addSpacing, _1, parser, lefinReader)])
         >> -SPACINGTABLE >> lit(";"));

  bool valid
      = qi::phrase_parse(first, last, spacingTableRule, space) && first == last;

  if (!valid) {  // fail if we did not get a full match
    odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
  } else {
    if (parser->spacing_tbl.size() != parser->width_tbl.size()) {
      valid = false;
    }
    for (const auto& spacing : parser->spacing_tbl) {
      if (spacing.size() != parser->length_tbl.size()) {
        valid = false;
      }
    }
    if (valid) {
      rule->setTable(parser->width_tbl,
                     parser->length_tbl,
                     parser->spacing_tbl,
                     parser->within_map);
      rule->setSpacingTableInfluence(parser->influence_tbl);
    } else {
      odb::dbTechLayerSpacingTablePrlRule::destroy(rule);
    }
  }
  return valid;
}
}  // namespace odb::lefTechLayerSpacingTablePrl

namespace odb {

bool lefTechLayerSpacingTablePrlParser::parse(const std::string& s,
                                              dbTechLayer* layer,
                                              odb::lefinReader* l)
{
  return lefTechLayerSpacingTablePrl::parse(s.begin(), s.end(), layer, l, this);
}

}  // namespace odb
