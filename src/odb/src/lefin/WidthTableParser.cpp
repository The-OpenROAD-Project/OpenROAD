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

void WidthTableParser::addWidth(double width)
{
  rule_->addWidth(lefin_->dbdist(width));
}

bool WidthTableParser::parseSubRule(std::string s)
{
  rule_ = dbTechLayerWidthTableRule::create(layer_);
  qi::rule<std::string::iterator, space_type> LEF58_WIDTHTABLE
      = (lit("WIDTHTABLE")
         >> +double_[boost::bind(&WidthTableParser::addWidth, this, _1)]
         >> -lit("WRONGDIRECTION")[boost::bind(
             &dbTechLayerWidthTableRule::setWrongDirection, rule_, true)]
         >> -lit("ORTHOGONAL")[boost::bind(
             &dbTechLayerWidthTableRule::setOrthogonal, rule_, true)]
         >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, LEF58_WIDTHTABLE, space) && first == last;

  if (!valid && rule_ != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerWidthTableRule::destroy(rule_);
  }
  return valid;
}

void WidthTableParser::parse(const std::string& s)
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
      lefin_->warning(279,
                      "parse mismatch in layer property LEF58_WIDTHTABLE for "
                      "layer {} :\"{}\"",
                      layer_->getName(),
                      rule);
    }
  }
}
