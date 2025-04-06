// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <functional>
#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

using namespace odb;

void ArraySpacingParser::setCutClass(std::string class_name)
{
  auto cut_class = layer_->findTechLayerCutClassRule(class_name.c_str());
  if (cut_class != nullptr) {
    rule_->setCutClass(cut_class);
  }
}

void ArraySpacingParser::setArraySpacing(
    boost::fusion::vector<int, double>& params)
{
  auto cuts = at_c<0>(params);
  auto spacing = at_c<1>(params);
  rule_->setCutsArraySpacing(cuts, lefin_->dbdist(spacing));
}

void ArraySpacingParser::setWithin(
    boost::fusion::vector<double, double>& params)
{
  auto within = at_c<0>(params);
  auto arraywidth = at_c<1>(params);
  rule_->setWithin(lefin_->dbdist(within));
  rule_->setArrayWidth(lefin_->dbdist(arraywidth));
  rule_->setWithinValid(true);
}

void ArraySpacingParser::setCutSpacing(double spacing)
{
  rule_->setCutSpacing(lefin_->dbdist(spacing));
}

void ArraySpacingParser::setViaWidth(double width)
{
  rule_->setViaWidth(lefin_->dbdist(width));
  rule_->setViaWidthValid(true);
}

bool ArraySpacingParser::parse(std::string s)
{
  rule_ = dbTechLayerArraySpacingRule::create(layer_);

  qi::rule<std::string::iterator, space_type> CUTCLASS
      = (lit("CUTCLASS")
         >> _string[boost::bind(&ArraySpacingParser::setCutClass, this, _1)]);
  qi::rule<std::string::iterator, space_type> VIA_WIDTH
      = (lit("WIDTH")
         >> double_[boost::bind(&ArraySpacingParser::setViaWidth, this, _1)]);
  qi::rule<std::string::iterator, space_type> ARRAYCUTS
      = (lit("ARRAYCUTS") >> int_ >> lit("SPACING") >> double_)[boost::bind(
          &ArraySpacingParser::setArraySpacing, this, _1)];
  qi::rule<std::string::iterator, space_type> WITHIN
      = (lit("WITHIN") >> double_ >> lit("ARRAYWIDTH")
         >> double_)[boost::bind(&ArraySpacingParser::setWithin, this, _1)];

  qi::rule<std::string::iterator, space_type> LEF58_ARRAYSPACING
      = (lit("ARRAYSPACING") >> -(CUTCLASS)
         >> -lit("PARALLELOVERLAP")[boost::bind(
             &dbTechLayerArraySpacingRule::setParallelOverlap, rule_, true)]
         >> -lit("LONGARRAY")[boost::bind(
             &dbTechLayerArraySpacingRule::setLongArray, rule_, true)]
         >> -VIA_WIDTH >> -WITHIN >> lit("CUTSPACING")
         >> double_[boost::bind(&ArraySpacingParser::setCutSpacing, this, _1)]
         >> +ARRAYCUTS >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, LEF58_ARRAYSPACING, space)
               && first == last;

  if (!valid && rule_ != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerArraySpacingRule::destroy(rule_);
  }
  return valid;
}
