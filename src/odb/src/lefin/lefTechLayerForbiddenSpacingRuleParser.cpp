// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerForbiddenSpacingRuleParser::lefTechLayerForbiddenSpacingRuleParser(
    lefinReader* l)
{
  lefin_ = l;
}

void lefTechLayerForbiddenSpacingRuleParser::parse(std::string s,
                                                   odb::dbTechLayer* layer)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto& rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty()) {
      continue;
    }
    rule += " ; ";
    if (!parseSubRule(rule, layer)) {
      lefin_->warning(
          438,
          "parse mismatch in layer property LEF58_FORBIDDENSPACING for "
          "layer {} :\"{}\"",
          layer->getName(),
          rule);
    }
  }
}

void lefTechLayerForbiddenSpacingRuleParser::setInt(
    double val,
    odb::dbTechLayerForbiddenSpacingRule* rule,
    void (odb::dbTechLayerForbiddenSpacingRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}

void lefTechLayerForbiddenSpacingRuleParser::setForbiddenSpacing(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerForbiddenSpacingRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setForbiddenSpacing(size);
}

bool lefTechLayerForbiddenSpacingRuleParser::parseSubRule(
    std::string s,
    odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  odb::dbTechLayerForbiddenSpacingRule* rule
      = odb::dbTechLayerForbiddenSpacingRule::create(layer);

  qi::rule<std::string::iterator, space_type> forbiddenSpacing
      = (lit("FORBIDDENSPACING") >> double_ >> double_)[boost::bind(
            &lefTechLayerForbiddenSpacingRuleParser::setForbiddenSpacing,
            this,
            _1,
            rule)]
        >> -(lit("WIDTH") >> double_[boost::bind(
                 &lefTechLayerForbiddenSpacingRuleParser::setInt,
                 this,
                 _1,
                 rule,
                 &odb::dbTechLayerForbiddenSpacingRule::setWidth)]
             >> -(lit("WITHIN") >> double_[boost::bind(
                      &lefTechLayerForbiddenSpacingRuleParser::setInt,
                      this,
                      _1,
                      rule,
                      &odb::dbTechLayerForbiddenSpacingRule::setWithin)])
             >> -(lit("PRL") >> double_[boost::bind(
                      &lefTechLayerForbiddenSpacingRuleParser::setInt,
                      this,
                      _1,
                      rule,
                      &odb::dbTechLayerForbiddenSpacingRule::setPrl)])
             >> -(lit("TWOEDGES") >> double_[boost::bind(
                      &lefTechLayerForbiddenSpacingRuleParser::setInt,
                      this,
                      _1,
                      rule,
                      &odb::dbTechLayerForbiddenSpacingRule::setTwoEdges)])
             >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, forbiddenSpacing, space) && first == last;
  if (!valid) {
    odb::dbTechLayerForbiddenSpacingRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
