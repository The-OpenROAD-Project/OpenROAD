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

#include <functional>
#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

using namespace odb;

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
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto& rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty()) {
      continue;
    }
    rule += " ; ";
    if (!parseSubRule(rule)) {
      lefin_->warning(299,
                      "parse mismatch in layer propery LEF58_MINIMUMCUT for "
                      "layer {} :\"{}\"",
                      layer_->getName(),
                      rule);
    }
  }
}

bool MinCutParser::parseSubRule(std::string s)
{
  rule_ = dbTechLayerMinCutRule::create(layer_);
  qi::rule<std::string::iterator, space_type> CUTCLASS
      = (lit("CUTCLASS") >> _string
         >> int_)[boost::bind(&MinCutParser::addCutClass, this, _1)];
  qi::rule<std::string::iterator, space_type> WITHIN
      = (lit("WITHIN")
         >> double_)[boost::bind(&MinCutParser::setWithinCutDist, this, _1)];
  qi::rule<std::string::iterator, space_type> LENGTH
      = (lit("LENGTH")
         >> double_[boost::bind(&MinCutParser::setLength, this, _1)]
         >> lit("WITHIN")
         >> double_[boost::bind(&MinCutParser::setLengthWithin, this, _1)]);
  qi::rule<std::string::iterator, space_type> AREA
      = (lit("AREA") >> double_[boost::bind(&MinCutParser::setArea, this, _1)]
         >> -(lit("WITHIN")
              >> double_[boost::bind(&MinCutParser::setAreaWithin, this, _1)]));
  qi::rule<std::string::iterator, space_type> LEF58_MINCUT
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
