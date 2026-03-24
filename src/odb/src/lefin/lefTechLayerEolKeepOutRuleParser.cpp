// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

lefTechLayerEolKeepOutRuleParser::lefTechLayerEolKeepOutRuleParser(
    lefinReader* l)
{
  lefin_ = l;
}

void lefTechLayerEolKeepOutRuleParser::parse(const std::string& s,
                                             odb::dbTechLayer* layer)
{
  processRules(s, [this, layer](const std::string& rule) {
    if (!parseSubRule(rule, layer)) {
      lefin_->warning(280,
                      "parse mismatch in layer property LEF58_EOLKEEPOUT for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
    }
  });
}
void lefTechLayerEolKeepOutRuleParser::setClass(
    const std::string& val,
    odb::dbTechLayerEolKeepOutRule* rule,
    odb::dbTechLayer* layer)
{
  rule->setClassName(val);
  rule->setClassValid(true);
}

void lefTechLayerEolKeepOutRuleParser::setExceptWithin(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerEolKeepOutRule* rule)
{
  rule->setExceptWithin(true);
  rule->setWithinLow(lefin_->dbdist(at_c<0>(params)));
  rule->setWithinHigh(lefin_->dbdist(at_c<1>(params)));
}

void lefTechLayerEolKeepOutRuleParser::setInt(
    double val,
    odb::dbTechLayerEolKeepOutRule* rule,
    void (odb::dbTechLayerEolKeepOutRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}
bool lefTechLayerEolKeepOutRuleParser::parseSubRule(const std::string& s,
                                                    odb::dbTechLayer* layer)
{
  odb::dbTechLayerEolKeepOutRule* rule
      = odb::dbTechLayerEolKeepOutRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> EXCEPTWITHIN
      = (lit("EXCEPTWITHIN") >> double_ >> double_)[boost::bind(
          &lefTechLayerEolKeepOutRuleParser::setExceptWithin, this, _1, rule)];
  qi::rule<std::string::const_iterator, space_type> CLASS
      = (lit("CLASS")
         >> _string[boost::bind(&lefTechLayerEolKeepOutRuleParser::setClass,
                                this,
                                _1,
                                rule,
                                layer)]);
  qi::rule<std::string::const_iterator, space_type> EOLKEEPOUT
      = (lit("EOLKEEPOUT")
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setEolWidth)]
         >> lit("EXTENSION") >> double_[boost::bind(
             &lefTechLayerEolKeepOutRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerEolKeepOutRule::setBackwardExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setSideExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setForwardExt)]
         >> -EXCEPTWITHIN >> -CLASS >> -lit("CORNERONLY")[boost::bind(
             &odb::dbTechLayerEolKeepOutRule::setCornerOnly, rule, true)]
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, EOLKEEPOUT, space) && first == last;
  if (!valid) {
    odb::dbTechLayerEolKeepOutRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
