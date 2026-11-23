// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string>
#include <vector>

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

using VoltageSpacingTables = std::vector<odb::dbTechLayerVoltageSpacing*>;

void createTable(odb::dbTechLayer* layer, VoltageSpacingTables* tables)
{
  tables->push_back(odb::dbTechLayerVoltageSpacing::create(layer));
}

void addEntry(VoltageSpacingTables* tables,
              lefinReader* lefin,
              boost::fusion::vector<double, double>& params)
{
  tables->back()->addEntry(at_c<0>(params), lefin->dbdist(at_c<1>(params)));
}

void setFlags(VoltageSpacingTables* tables, boost::optional<std::string>& tocut)
{
  odb::dbTechLayerVoltageSpacing* table = tables->back();
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
  // A single property may hold several VOLTAGESPACING statements, each
  // becoming its own dbTechLayerVoltageSpacing.
  VoltageSpacingTables tables;

  qi::rule<std::string::const_iterator, space_type> ENTRY
      = (double_ >> double_)[boost::bind(&addEntry, &tables, lefinReader, _1)];
  qi::rule<std::string::const_iterator, space_type> VOLTAGESPACING
      = lit("VOLTAGESPACING")[boost::bind(&createTable, layer, &tables)]
        >> -((lit("TOCUT")
              >> -(string("ABOVE")
                   | string("BELOW")))[boost::bind(&setFlags, &tables, _1)])
        >> +ENTRY >> lit(";");
  bool valid = qi::phrase_parse(first, last, +VOLTAGESPACING, space);

  if (!valid || first != last) {
    for (odb::dbTechLayerVoltageSpacing* table : tables) {
      odb::dbTechLayerVoltageSpacing::destroy(table);
    }

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
