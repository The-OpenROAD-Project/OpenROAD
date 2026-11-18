// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerBackside {

namespace {

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* /*lefinReader*/)
{
  qi::rule<Iterator, space_type> backsideRule
      = (lit("BACKSIDE")[boost::bind(
             &odb::dbTechLayer::setBackside, layer, true)]
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, backsideRule, space);
  return valid && first == last;
}

}  // namespace

}  // namespace odb::lefTechLayerBackside

namespace odb {

bool lefTechLayerBacksideParser::parse(const std::string& s,
                                       dbTechLayer* layer,
                                       odb::lefinReader* l)
{
  return lefTechLayerBackside::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
