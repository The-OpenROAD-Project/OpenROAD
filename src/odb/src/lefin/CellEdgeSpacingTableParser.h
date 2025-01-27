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

#pragma once

#include <boost/fusion/container.hpp>
#include <boost/optional/optional.hpp>
#include <boost/spirit/include/support_unused.hpp>

#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

class CellEdgeSpacingTableParser
{
 public:
  CellEdgeSpacingTableParser(dbTech* tech, lefinReader* lefin)
      : tech_(tech), lefin_(lefin)
  {
  }
  void parse(const std::string&);

 private:
  bool parseEntry(std::string);
  void createEntry();
  void setSpacing(double);
  void setBool(void (dbCellEdgeSpacing::*func)(bool));
  void setString(const std::string& val,
                 void (dbCellEdgeSpacing::*func)(const std::string&));
  dbTech* tech_;
  lefinReader* lefin_;
  dbCellEdgeSpacing* curr_entry_{nullptr};
};
}  // namespace odb
