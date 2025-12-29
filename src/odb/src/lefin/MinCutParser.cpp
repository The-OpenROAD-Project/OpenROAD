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

void MinCutParser::addCutClass(boost::fusion::vector<std::string, int>& params)
{
  auto className = at_c<0>(params);
  auto numCuts = at_c<1>(params);
  rule_->setCutsPerCutClass(className, numCuts);
}

void MinCutParser::setWidth(double width)
{
  rule_->setWidth(lefin_->dbdist(width));
}

void MinCutParser::setWithinCutDist(double within)
{
  rule_->setWithinCutDist(lefin_->dbdist(within));
  rule_->setWithinCutDistValid(true);
}

void MinCutParser::setLength(double length)
{
  rule_->setLength(lefin_->dbdist(length));
  rule_->setLengthValid(true);
}

void MinCutParser::setLengthWithin(double within)
{
  rule_->setLengthWithinDist(lefin_->dbdist(within));
}

void MinCutParser::setArea(double area)
{
  rule_->setArea(lefin_->dbdist(area));
  rule_->setAreaValid(true);
}

void MinCutParser::setAreaWithin(double within)
{
  rule_->setAreaWithinDist(lefin_->dbdist(within));
  rule_->setAreaWithinDistValid(true);
}

void MinCutParser::parse(const std::string& s)
{
  processRules(s, [this](const std::string& rule) {
    if (!parseSubRule(rule)) {
      lefin_->warning(299,
                      "parse mismatch in layer property LEF58_MINIMUMCUT for "
                      "layer {} :\"{}\"",
                      layer_->getName(),
                      rule);
    }
  });
}

bool MinCutParser::parseSubRule(const std::string& s)
{
  rule_ = dbTechLayerMinCutRule::create(layer_);
  qi::rule<std::string::const_iterator, space_type> CUTCLASS
      = (lit("CUTCLASS") >> _string
         >> int_)[boost::bind(&MinCutParser::addCutClass, this, _1)];
  qi::rule<std::string::const_iterator, space_type> WITHIN
      = (lit("WITHIN")
         >> double_)[boost::bind(&MinCutParser::setWithinCutDist, this, _1)];
  qi::rule<std::string::const_iterator, space_type> LENGTH
      = (lit("LENGTH")
         >> double_[boost::bind(&MinCutParser::setLength, this, _1)]
         >> lit("WITHIN")
         >> double_[boost::bind(&MinCutParser::setLengthWithin, this, _1)]);
  qi::rule<std::string::const_iterator, space_type> AREA
      = (lit("AREA") >> double_[boost::bind(&MinCutParser::setArea, this, _1)]
         >> -(lit("WITHIN")
              >> double_[boost::bind(&MinCutParser::setAreaWithin, this, _1)]));
  qi::rule<std::string::const_iterator, space_type> LEF58_MINCUT
      = (lit("MINIMUMCUT")
         >> (int_[boost::bind(&dbTechLayerMinCutRule::setNumCuts, rule_, _1)]
             | +CUTCLASS)
         >> lit("WIDTH")
         >> double_[boost::bind(&MinCutParser::setWidth, this, _1)] >> -WITHIN
         >> -(lit("FROMABOVE")[boost::bind(
                  &dbTechLayerMinCutRule::setFromAbove, rule_, true)]
              | lit("FROMBELOW")[boost::bind(
                  &dbTechLayerMinCutRule::setFromBelow, rule_, true)])
         >> -(LENGTH | AREA
              | lit("SAMEMETALOVERLAP")[boost::bind(
                  &dbTechLayerMinCutRule::setSameMetalOverlap, rule_, true)]
              | lit("FULLYENCLOSED")[boost::bind(
                  &dbTechLayerMinCutRule::setFullyEnclosed, rule_, true)])
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, LEF58_MINCUT, space) && first == last;

  if (!valid && rule_ != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerMinCutRule::destroy(rule_);
  }
  return valid;
}

}  // namespace odb
