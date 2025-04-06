// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <functional>
#include <string>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerTwoWiresForbiddenSpcRuleParser::
    lefTechLayerTwoWiresForbiddenSpcRuleParser(lefinReader* l)
{
  lefin_ = l;
}

void lefTechLayerTwoWiresForbiddenSpcRuleParser::parse(std::string s,
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

void lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt(
    double val,
    odb::dbTechLayerTwoWiresForbiddenSpcRule* rule,
    void (odb::dbTechLayerTwoWiresForbiddenSpcRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}

void lefTechLayerTwoWiresForbiddenSpcRuleParser::setForbiddenSpacing(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerTwoWiresForbiddenSpcRule* rule)
{
  rule->setMinSpacing(lefin_->dbdist(at_c<0>(params)));
  rule->setMaxSpacing(lefin_->dbdist(at_c<1>(params)));
}

bool lefTechLayerTwoWiresForbiddenSpcRuleParser::parseSubRule(
    std::string s,
    odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  odb::dbTechLayerTwoWiresForbiddenSpcRule* rule
      = odb::dbTechLayerTwoWiresForbiddenSpcRule::create(layer);

  qi::rule<std::string::iterator, space_type> forbiddenSpacing
      = (lit("TWOWIRESFORBIDDENSPACING") >> double_ >> double_)[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setForbiddenSpacing,
            this,
            _1,
            rule)]
        >> lit("MINSPANLENGTH") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMinSpanLength)]
        >> -lit("EXACTSPANLENGTH")[boost::bind(
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMinExactSpanLength,
            rule,
            true)]
        >> lit("MAXSPANLENGTH") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpanLength)]
        >> -lit("EXACTSPANLENGTH")[boost::bind(
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMaxExactSpanLength,
            rule,
            true)]
        >> lit("PRL") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setPrl)]
        >> lit(";");
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, forbiddenSpacing, space) && first == last;
  if (!valid) {
    odb::dbTechLayerTwoWiresForbiddenSpcRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
