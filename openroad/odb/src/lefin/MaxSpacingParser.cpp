// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

bool MaxSpacingParser::parseSubRule(const std::string& s)
{
  dbTechLayerMaxSpacingRule* rule = dbTechLayerMaxSpacingRule::create(layer_);
  qi::rule<std::string::const_iterator, space_type> LEF58_MAXSPACING;
  if (layer_->getType() == dbTechLayerType::IMPLANT) {
    // LEF 5.8 IMPLANT syntax: MAXSPACING maxSpacing WIDTH width LENGTH length ;
    LEF58_MAXSPACING
        = (lit("MAXSPACING") >> double_[dbdistSetter(
               &dbTechLayerMaxSpacingRule::setMaxSpacing, rule, lefin_)]
           >> lit("WIDTH") >> double_[dbdistSetter(
               &dbTechLayerMaxSpacingRule::setWidth, rule, lefin_)]
           >> lit("LENGTH") >> double_[dbdistSetter(
               &dbTechLayerMaxSpacingRule::setLength, rule, lefin_)]
           >> lit(";"));
  } else {
    LEF58_MAXSPACING
        = (lit("MAXSPACING") >> double_[dbdistSetter(
               &dbTechLayerMaxSpacingRule::setMaxSpacing, rule, lefin_)]
           >> -(lit("CUTCLASS") >> _string[boost::bind(
                    &dbTechLayerMaxSpacingRule::setCutClass, rule, _1)])
           >> lit(";"));
  }

  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, LEF58_MAXSPACING, space) && first == last;

  if (!valid && rule != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerMaxSpacingRule::destroy(rule);
  }
  return valid;
}

void MaxSpacingParser::parse(const std::string& s)
{
  if (!parseAllRules(
          s, [this](const std::string& rule) { return parseSubRule(rule); })) {
    lefin_->warning(279,
                    "parse mismatch in layer property LEF58_MAXSPACING for "
                    "layer {} :\"{}\"",
                    layer_->getName(),
                    s);
  }
}

}  // namespace odb
