// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string_view>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefMacroPropParser.h"
#include "odb/db.h"

namespace odb {

bool lefMacroBacksideBridgeParser::parse(std::string_view s, dbMaster* master)
{
  qi::rule<std::string_view::iterator, space_type> rule
      = (lit("BACKSIDEBRIDGE")[boost::bind(
             &dbMaster::setBacksideBridge, master, true)]
         >> lit(";"));

  auto first = s.begin();
  const bool valid = qi::phrase_parse(first, s.end(), rule, space);
  return valid && first == s.end();
}

}  // namespace odb
