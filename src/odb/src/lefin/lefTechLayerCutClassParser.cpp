// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCutClass {

void addCutClassRule(
    boost::fusion::vector<std::string,
                          double,
                          boost::optional<double>,
                          boost::optional<int>,
                          boost::optional<std::string>>& params,
    odb::dbTechLayer* layer,
    odb::lefinReader* lefin)
{
  std::string name = at_c<0>(params);
  auto rule = odb::dbTechLayerCutClassRule::create(layer, name.c_str());
  rule->setWidth(lefin->dbdist(at_c<1>(params)));
  auto length = at_c<2>(params);
  auto cnt = at_c<3>(params);
  auto orient = at_c<4>(params);
  if (length.is_initialized()) {
    rule->setLengthValid(true);
    rule->setLength(lefin->dbdist(length.value()));
  }
  if (cnt.is_initialized()) {
    rule->setCutsValid(true);
    rule->setNumCuts(cnt.value());
  }
  if (orient.is_initialized()) {
    lefin->warning(
        421, "Keyword ORIENT is not supported in CUTCLASS {}.", name);
  }
}

}  // namespace odb::lefTechLayerCutClass

namespace odb {
bool lefTechLayerCutClassParser::parse(std::string s,
                                       dbTechLayer* layer,
                                       odb::lefinReader* lefin)
{
  auto first = s.begin();
  auto last = s.end();
  qi::rule<std::string::iterator, space_type> cutClassRule
      = (+(lit("CUTCLASS") >> _string >> lit("WIDTH") >> double_
           >> -(lit("LENGTH") >> double_) >> -(lit("CUTS") >> int_)
           >> -(lit("ORIENT") >> _string) >> lit(";"))[boost::bind(
          &lefTechLayerCutClass::addCutClassRule, _1, layer, lefin)]);

  bool valid
      = qi::phrase_parse(first, last, cutClassRule, space) && first == last;
  return valid;
}

}  // namespace odb
