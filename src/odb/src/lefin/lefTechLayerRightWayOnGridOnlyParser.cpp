// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerRightWayOnGridOnly {

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader)
{
  qi::rule<std::string::iterator, space_type> rightWayOnGridOnlyRule
      = (lit("RIGHTWAYONGRIDONLY")[boost::bind(
             &odb::dbTechLayer::setRightWayOnGridOnly, layer, true)]
         >> -lit("CHECKMASK")[boost::bind(
             &odb::dbTechLayer::setRightWayOnGridOnlyCheckMask, layer, true)]
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, rightWayOnGridOnlyRule, space);
  return valid && first == last;
}
}  // namespace odb::lefTechLayerRightWayOnGridOnly

namespace odb {

bool lefTechLayerRightWayOnGridOnlyParser::parse(std::string s,
                                                 dbTechLayer* layer,
                                                 odb::lefinReader* l)
{
  return lefTechLayerRightWayOnGridOnly::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
