// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Parser for LEF58 edge type rules that define edge properties for macro cells
#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefMacroPropParser.h"
#include "parserUtils.h"

namespace odb {

// Parse input string containing edge type rules for a macro
void lefMacroEdgeTypeParser::parse(const std::string& s)
{
  processRules(s, [this](const std::string& rule) {
    if (!parseSubRule(rule)) {
      lefin_->warning(299,
                      "parse mismatch in layer property LEF58_EDGETYPE for "
                      "macro {} :\"{}\"",
                      master_->getName(),
                      rule);
    }
  });
}

// Set range parameters for the edge type (converts to database units)
void lefMacroEdgeTypeParser::setRange(
    boost::fusion::vector<double, double>& params)
{
  double begin = at_c<0>(params);
  double end = at_c<1>(params);
  edge_type_->setRangeBegin(lefin_->dbdist(begin));
  edge_type_->setRangeEnd(lefin_->dbdist(end));
}

// Parse a single edge type rule
// Format: EDGETYPE (RIGHT|LEFT|TOP|BOTTOM) type [CELLROW row] [HALFROW row]
// [RANGE begin end] ;
bool lefMacroEdgeTypeParser::parseSubRule(const std::string& s)
{
  edge_type_ = dbMasterEdgeType::create(master_);
  qi::rule<std::string::const_iterator, space_type> EDGE_DIR
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
  qi::rule<std::string::const_iterator, space_type> CELLROW
      = (lit("CELLROW")
         >> int_[boost::bind(&dbMasterEdgeType::setCellRow, edge_type_, _1)]);
  qi::rule<std::string::const_iterator, space_type> HALFROW
      = (lit("HALFROW")
         >> int_[boost::bind(&dbMasterEdgeType::setHalfRow, edge_type_, _1)]);
  qi::rule<std::string::const_iterator, space_type> RANGE
      = (lit("RANGE") >> double_
         >> double_)[boost::bind(&lefMacroEdgeTypeParser::setRange, this, _1)];
  qi::rule<std::string::const_iterator, space_type> LEF58_EDGETYPE
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
