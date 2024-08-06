/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, Precision Innovations Inc.
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

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
