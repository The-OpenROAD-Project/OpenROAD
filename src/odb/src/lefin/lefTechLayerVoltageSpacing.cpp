// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boost/fusion/container/vector/vector.hpp"
#include "boost/optional/optional.hpp"
#include "boost/spirit/home/qi/detail/parse_auto.hpp"
#include "boost/spirit/home/qi/nonterminal/rule.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

namespace {

void addEntry(odb::dbTechLayerVoltageSpacing* table,
              lefinReader* lefin,
              boost::fusion::vector<double, double>& params)
{
  table->addEntry(at_c<0>(params), lefin->dbdist(at_c<1>(params)));
}

void setFlags(odb::dbTechLayerVoltageSpacing* table,
              boost::optional<std::string>& tocut)
{
  if (tocut.is_initialized()) {
    if (tocut.get() == "ABOVE") {
      table->setTocutAbove(true);
    } else if (tocut.get() == "BELOW") {
      table->setTocutBelow(true);
    }
  } else {
    table->setTocutAbove(true);
    table->setTocutBelow(true);
  }
}

template <typename Iterator>
bool parseRule(Iterator first,
               Iterator last,
               odb::dbTechLayer* layer,
               odb::lefinReader* lefinReader)
{
  odb::dbTechLayerVoltageSpacing* table
      = odb::dbTechLayerVoltageSpacing::create(layer);

  qi::rule<std::string::const_iterator, space_type> ENTRY
      = (double_ >> double_)[boost::bind(&addEntry, table, lefinReader, _1)];
  qi::rule<std::string::const_iterator, space_type> VOLTAGESPACING
      = lit("VOLTAGESPACING")
        >> -((lit("TOCUT")
              >> -(string("ABOVE")
                   | string("BELOW")))[boost::bind(&setFlags, table, _1)])
        >> +ENTRY >> lit(";");
  bool valid = qi::phrase_parse(first, last, VOLTAGESPACING, space);

  if (!valid || first != last) {
    odb::dbTechLayerVoltageSpacing::destroy(table);

    return false;
  }

  return true;
}
}  // namespace

void lefTechLayerVoltageSpacing::parse(const std::string& s)
{
  if (!parseRule(s.begin(), s.end(), layer_, lefin_)) {
    lefin_->warning(500,
                    "parse mismatch in layer property LEF58_VOLTAGESPACING for "
                    "layer {} :\"{}\"",
                    layer_->getName(),
                    s);
  }
}

}  // namespace odb
