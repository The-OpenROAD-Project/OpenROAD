// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <functional>
#include <iostream>
#include <string>
#include <vector>

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
                      "parse mismatch in layer property LEF58_KEEPOUTZONE for "
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
