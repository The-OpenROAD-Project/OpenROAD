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

void KeepOutZoneParser::setInt(
    double val,
    void (odb::dbTechLayerKeepOutZoneRule::*func)(int))
{
  (rule_->*func)(lefin_->dbdist(val));
}

void KeepOutZoneParser::parse(const std::string& s)
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
      lefin_->warning(388,
                      "parse mismatch in layer propery LEF58_KEEPOUTZONE for "
                      "layer {} :\"{}\"",
                      layer_->getName(),
                      rule);
    }
  }
}

bool KeepOutZoneParser::parseSubRule(std::string s)
{
  rule_ = dbTechLayerKeepOutZoneRule::create(layer_);
  qi::rule<std::string::iterator, space_type> EXCEPTEXACTALIGNED
      = (lit("EXCEPTEXACTALIGNED")
         >> -(lit("SIDE")[boost::bind(
                  &dbTechLayerKeepOutZoneRule::setExceptAlignedSide,
                  rule_,
                  true)]
              | lit("END")[boost::bind(
                  &dbTechLayerKeepOutZoneRule::setExceptAlignedEnd,
                  rule_,
                  true)])
         >> double_[boost::bind(
             &KeepOutZoneParser::setInt,
             this,
             _1,
             &odb::dbTechLayerKeepOutZoneRule::setAlignedSpacing)]);
  qi::rule<std::string::iterator, space_type> EXTENSION
      = ((lit("EXTENSION") >> double_[boost::bind(
              &KeepOutZoneParser::setInt,
              this,
              _1,
              &odb::dbTechLayerKeepOutZoneRule::setSideExtension)]
          >> double_[boost::bind(
              &KeepOutZoneParser::setInt,
              this,
              _1,
              &odb::dbTechLayerKeepOutZoneRule::setForwardExtension)])
         | (lit("ENDEXTENSION") >> double_[boost::bind(
                &KeepOutZoneParser::setInt,
                this,
                _1,
                &odb::dbTechLayerKeepOutZoneRule::setEndSideExtension)]
            >> double_[boost::bind(
                &KeepOutZoneParser::setInt,
                this,
                _1,
                &odb::dbTechLayerKeepOutZoneRule::setEndForwardExtension)]
            >> lit("SIDEEXTENSION") >> double_[boost::bind(
                &KeepOutZoneParser::setInt,
                this,
                _1,
                &odb::dbTechLayerKeepOutZoneRule::setSideSideExtension)]
            >> double_[boost::bind(
                &KeepOutZoneParser::setInt,
                this,
                _1,
                &odb::dbTechLayerKeepOutZoneRule::setSideForwardExtension)]));
  qi::rule<std::string::iterator, space_type> SPIRALEXTENSION
      = ((lit("SPIRALEXTENSION") >> double_[boost::bind(
              &KeepOutZoneParser::setInt,
              this,
              _1,
              &odb::dbTechLayerKeepOutZoneRule::setSpiralExtension)]));

  qi::rule<std::string::iterator, space_type> LEF58_KEEPOUTZONE
      = (lit("KEEPOUTZONE") >> lit("CUTCLASS") >> _string[boost::bind(
             &dbTechLayerKeepOutZoneRule::setFirstCutClass, rule_, _1)]
         >> -(lit("TO") >> _string[boost::bind(
                  &dbTechLayerKeepOutZoneRule::setSecondCutClass, rule_, _1)])
         >> -EXCEPTEXACTALIGNED >> EXTENSION >> SPIRALEXTENSION >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, LEF58_KEEPOUTZONE, space)
               && first == last;

  if (!valid && rule_ != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerKeepOutZoneRule::destroy(rule_);
  }
  return valid;
}
