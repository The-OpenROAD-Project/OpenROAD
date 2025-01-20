/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025, Precision Innovations Inc.
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
                      "parse mismatch in propery LEF58_CELLEDGESPACINGTABLE");
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