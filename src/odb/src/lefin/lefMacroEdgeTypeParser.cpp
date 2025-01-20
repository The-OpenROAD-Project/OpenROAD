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
#include <vector>

#include "boostParser.h"
#include "lefMacroPropParser.h"

namespace odb {
void lefMacroEdgeTypeParser::parse(const std::string& s)
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
                      "parse mismatch in layer propery LEF58_EDGETYPE for "
                      "macro {} :\"{}\"",
                      master_->getName(),
                      rule);
    }
  }
}

void lefMacroEdgeTypeParser::setRange(
    boost::fusion::vector<double, double>& params)
{
  double begin = at_c<0>(params);
  double end = at_c<1>(params);
  edge_type_->setRangeBegin(lefin_->dbdist(begin));
  edge_type_->setRangeEnd(lefin_->dbdist(end));
}

bool lefMacroEdgeTypeParser::parseSubRule(std::string s)
{
  edge_type_ = dbMasterEdgeType::create(master_);
  qi::rule<std::string::iterator, space_type> EDGE_DIR
      = (lit("RIGHT")[boost::bind(&dbMasterEdgeType::setEdgeDir,
                                  edge_type_,
                                  dbMasterEdgeType::RIGHT)]
         | lit("LEFT")[boost::bind(
             &dbMasterEdgeType::setEdgeDir, edge_type_, dbMasterEdgeType::LEFT)]
         | lit("TOP")[boost::bind(
             &dbMasterEdgeType::setEdgeDir, edge_type_, dbMasterEdgeType::TOP)]
         | lit("BOTTOM")[boost::bind(&dbMasterEdgeType::setEdgeDir,
                                     edge_type_,
                                     dbMasterEdgeType::BOTTOM)]);
  qi::rule<std::string::iterator, space_type> CELLROW
      = (lit("CELLROW")
         >> int_[boost::bind(&dbMasterEdgeType::setCellRow, edge_type_, _1)]);
  qi::rule<std::string::iterator, space_type> HALFROW
      = (lit("HALFROW")
         >> int_[boost::bind(&dbMasterEdgeType::setHalfRow, edge_type_, _1)]);
  qi::rule<std::string::iterator, space_type> RANGE
      = (lit("RANGE") >> double_
         >> double_)[boost::bind(&lefMacroEdgeTypeParser::setRange, this, _1)];
  qi::rule<std::string::iterator, space_type> LEF58_EDGETYPE
      = (lit("EDGETYPE") >> EDGE_DIR
         >> _string[boost::bind(&dbMasterEdgeType::setEdgeType, edge_type_, _1)]
         >> -(CELLROW | HALFROW | RANGE) >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, LEF58_EDGETYPE, space) && first == last;

  if (!valid && edge_type_ != nullptr) {  // fail if we did not get a full match
    odb::dbMasterEdgeType::destroy(edge_type_);
  }
  return valid;
}
}  // namespace odb