// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <functional>
#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

using namespace odb;

void MaxSpacingParser::setMaxSpacing(dbTechLayerMaxSpacingRule* rule,
                                     double spc)
{
  rule->setMaxSpacing(lefin_->dbdist(spc));
}

void MaxSpacingParser::parse(std::string s)
{
  dbTechLayerMaxSpacingRule* rule = dbTechLayerMaxSpacingRule::create(layer_);
  qi::rule<std::string::iterator, space_type> LEF58_MAXSPACING
      = (lit("MAXSPACING") >> double_[boost::bind(
             &MaxSpacingParser::setMaxSpacing, this, rule, _1)]
         >> -(lit("CUTCLASS") >> _string[boost::bind(
                  &dbTechLayerMaxSpacingRule::setCutClass, rule, _1)])
         >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, LEF58_MAXSPACING, space) && first == last;

  if (!valid && rule != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerMaxSpacingRule::destroy(rule);
    lefin_->warning(279,
                    "parse mismatch in layer property LEF58_MAXSPACING for "
                    "layer {} :\"{}\"",
                    layer_->getName(),
                    s);
  }
}
