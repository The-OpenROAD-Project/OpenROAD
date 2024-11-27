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
