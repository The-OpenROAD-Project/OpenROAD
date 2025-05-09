// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "CellEdgeSpacingTableParser.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "boostParser.h"

namespace odb {
void CellEdgeSpacingTableParser::parse(const std::string& s)
{
  std::vector<std::string> entries;
  boost::split(entries, s, boost::is_any_of(";"));
  for (auto& entry : entries) {
    boost::algorithm::trim(entry);
    if (entry.empty()) {
      continue;
    }
    entry += " ; ";
    if (!parseEntry(entry)) {
      lefin_->warning(299,
                      "parse mismatch in property LEF58_CELLEDGESPACINGTABLE");
    }
  }
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

bool CellEdgeSpacingTableParser::parseEntry(std::string s)
{
  qi::rule<std::string::iterator, space_type> ENTRY
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
  qi::rule<std::string::iterator, space_type> LEF58_CELLEDGESPACINGTABLE
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
