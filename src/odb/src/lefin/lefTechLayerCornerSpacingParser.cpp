// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCornerSpacing {

void setWithin(double value,
               odb::dbTechLayerCornerSpacingRule* rule,
               odb::lefinReader* lefinReader)
{
  rule->setCornerOnly(true);
  rule->setWithin(lefinReader->dbdist(value));
}
void setEolWidth(double value,
                 odb::dbTechLayerCornerSpacingRule* rule,
                 odb::lefinReader* lefinReader)
{
  rule->setExceptEol(true);
  rule->setEolWidth(lefinReader->dbdist(value));
}
void setJogLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefinReader* lefinReader)
{
  rule->setExceptJogLength(true);
  rule->setJogLength(lefinReader->dbdist(value));
}
void setEdgeLength(double value,
                   odb::dbTechLayerCornerSpacingRule* rule,
                   odb::lefinReader* lefinReader)
{
  rule->setEdgeLengthValid(true);
  rule->setEdgeLength(lefinReader->dbdist(value));
}
void setMinLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefinReader* lefinReader)
{
  rule->setMinLengthValid(true);
  rule->setMinLength(lefinReader->dbdist(value));
}
void setExceptNotchLength(double value,
                          odb::dbTechLayerCornerSpacingRule* rule,
                          odb::lefinReader* lefinReader)
{
  rule->setExceptNotchLengthValid(true);
  rule->setExceptNotchLength(lefinReader->dbdist(value));
}
void addSpacing(
    boost::fusion::vector<double, double, boost::optional<double>>& params,
    odb::dbTechLayerCornerSpacingRule* rule,
    odb::lefinReader* lefinReader)
{
  auto width = lefinReader->dbdist(at_c<0>(params));
  auto spacing1 = lefinReader->dbdist(at_c<1>(params));
  auto spacing2 = at_c<2>(params);
  if (spacing2.is_initialized()) {
    rule->addSpacing(width, spacing1, lefinReader->dbdist(spacing2.value()));
  } else {
    rule->addSpacing(width, spacing1, spacing1);
  }
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader)
{
  odb::dbTechLayerCornerSpacingRule* rule
      = odb::dbTechLayerCornerSpacingRule::create(layer);
  qi::rule<std::string::iterator, space_type> convexCornerRule
      = (lit("CONVEXCORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONVEXCORNER)]
         >> -(lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setSameMask, rule, true)])
         >> -((lit("CORNERONLY")
               >> double_[boost::bind(&setWithin, _1, rule, lefinReader)])
              | lit("CORNERTOCORNER")[boost::bind(
                  &odb::dbTechLayerCornerSpacingRule::setCornerToCorner,
                  rule,
                  true)])
         >> -(lit("EXCEPTEOL")
              >> double_[boost::bind(&setEolWidth, _1, rule, lefinReader)]
              >> -(lit("EXCEPTJOGLENGTH")
                   >> double_[boost::bind(&setJogLength, _1, rule, lefinReader)]
                   >> -(lit("EDGELENGTH") >> double_[boost::bind(
                            &setEdgeLength, _1, rule, lefinReader)])
                   >> -(lit("INCLUDELSHAPE")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setIncludeShape,
                       rule,
                       true)]))));
  qi::rule<std::string::iterator, space_type> concaveCornerRule
      = (lit("CONCAVECORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONCAVECORNER)]
         >> -(lit("MINLENGTH")
              >> double_[boost::bind(&setMinLength, _1, rule, lefinReader)]
              >> -(lit("EXCEPTNOTCH")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setExceptNotch,
                       rule,
                       true)]
                   >> -double_[boost::bind(
                       &setExceptNotchLength, _1, rule, lefinReader)])));
  qi::rule<std::string::iterator, space_type> exceptSameRule
      = (lit("EXCEPTSAMENET")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameNet, rule, true)]
         | lit("EXCEPTSAMEMETAL")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameMetal,
             rule,
             true)]);

  qi::rule<std::string::iterator, space_type> spacingRule
      = (lit("WIDTH") >> double_ >> lit("SPACING") >> double_
         >> -double_)[boost::bind(&addSpacing, _1, rule, lefinReader)];

  qi::rule<std::string::iterator, space_type> cornerSpacingRule
      = (lit("CORNERSPACING") >> (convexCornerRule | concaveCornerRule)
         >> -(exceptSameRule) >> +(spacingRule) >> lit(";"));

  bool valid = qi::phrase_parse(first, last, cornerSpacingRule, space)
               && first == last;

  if (!valid) {
    odb::dbTechLayerCornerSpacingRule::destroy(rule);
  }

  return valid;
}
}  // namespace odb::lefTechLayerCornerSpacing

namespace odb {

bool lefTechLayerCornerSpacingParser::parse(std::string s,
                                            dbTechLayer* layer,
                                            odb::lefinReader* l)
{
  return lefTechLayerCornerSpacing::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
