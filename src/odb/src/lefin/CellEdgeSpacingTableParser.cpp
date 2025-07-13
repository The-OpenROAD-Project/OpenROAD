// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Parser for LEF58 cell edge spacing table rules
#include "CellEdgeSpacingTableParser.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "boostParser.h"
#include "parserUtils.h"

namespace odb {

// Parse the input string containing cell edge spacing rules
void CellEdgeSpacingTableParser::parse(const std::string& s)
{
  processRules(s, [this](const std::string& rule) {
    if (!parseEntry(rule)) {
      lefin_->warning(
          299,
          "parse mismatch in property LEF58_CELLEDGESPACINGTABLE: \"{}\"",
          rule);
    }
  });
}

void CellEdgeSpacingTableParser::createEntry()
{
  curr_entry_ = odb::dbCellEdgeSpacing::create(tech_);
}

void CellEdgeSpacingTableParser::setSpacing(double spc)
{
  curr_entry_->setSpacing(lefin_->dbdist(spc));
}

void CellEdgeSpacingTableParser::setBool(void (dbCellEdgeSpacing::*func)(bool))
{
  (curr_entry_->*func)(true);
}

void CellEdgeSpacingTableParser::setString(
    const std::string& val,
    void (dbCellEdgeSpacing::*func)(const std::string&))
{
  (curr_entry_->*func)(val);
}

// Parse a single cell edge spacing rule entry
// Format: EDGETYPE <type1> <type2> [EXCEPTABUTTED] [EXCEPTNONFILLERINBETWEEN]
// [OPTIONAL] [SOFT] [EXACT] <spacing> ;
bool CellEdgeSpacingTableParser::parseEntry(const std::string& s)
{
  qi::rule<std::string::const_iterator, space_type> ENTRY
      = lit("EDGETYPE")[boost::bind(&CellEdgeSpacingTableParser::createEntry,
                                    this)]
        >> _string[boost::bind(&CellEdgeSpacingTableParser::setString,
                               this,
                               _1,
                               &dbCellEdgeSpacing::setFirstEdgeType)]
        >> _string[boost::bind(&CellEdgeSpacingTableParser::setString,
                               this,
                               _1,
                               &dbCellEdgeSpacing::setSecondEdgeType)]
        >> -lit(
            "EXCEPTABUTTED")[boost::bind(&CellEdgeSpacingTableParser::setBool,
                                         this,
                                         &dbCellEdgeSpacing::setExceptAbutted)]
        >> -lit("EXCEPTNONFILLERINBETWEEN")[boost::bind(
            &CellEdgeSpacingTableParser::setBool,
            this,
            &dbCellEdgeSpacing::setExceptNonFillerInBetween)]
        >> -lit("OPTIONAL")[boost::bind(&CellEdgeSpacingTableParser::setBool,
                                        this,
                                        &dbCellEdgeSpacing::setOptional)]
        >> -lit("SOFT")[boost::bind(&CellEdgeSpacingTableParser::setBool,
                                    this,
                                    &dbCellEdgeSpacing::setSoft)]
        >> -lit("EXACT")[boost::bind(&CellEdgeSpacingTableParser::setBool,
                                     this,
                                     &dbCellEdgeSpacing::setExact)]
        >> double_[boost::bind(
            &CellEdgeSpacingTableParser::setSpacing, this, _1)];
  qi::rule<std::string::const_iterator, space_type> LEF58_CELLEDGESPACINGTABLE
      = (lit("CELLEDGESPACINGTABLE") >> -lit("NODEFAULT") >> +ENTRY
         >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, LEF58_CELLEDGESPACINGTABLE, space)
               && first == last;
  if (!valid
      && curr_entry_ != nullptr) {  // fail if we did not get a full match
    odb::dbCellEdgeSpacing::destroy(curr_entry_);
  }
  return valid;
}
}  // namespace odb
