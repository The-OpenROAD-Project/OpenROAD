// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerWrongDirSpacing {

void wrongDirParser(double value,
                    odb::dbTechLayerWrongDirSpacingRule* sc,
                    odb::lefinReader* l)
{
  sc->setWrongdirSpace(l->dbdist(value));
}

void noneolWidthParser(double value,
                       odb::dbTechLayerWrongDirSpacingRule* sc,
                       odb::lefinReader* l)
{
  sc->setNoneolWidth(l->dbdist(value));
}

void prlLengthParser(double value,
                     odb::dbTechLayerWrongDirSpacingRule* sc,
                     odb::lefinReader* l)
{
  sc->setPrlLength(l->dbdist(value));
}

void lengthParser(double value,
                  odb::dbTechLayerWrongDirSpacingRule* sc,
                  odb::lefinReader* l)
{
  sc->setLength(l->dbdist(value));
}

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* l)
{
  odb::dbTechLayerWrongDirSpacingRule* sc
      = odb::dbTechLayerWrongDirSpacingRule::create(layer);
  qi::rule<std::string::iterator, space_type> wrongDirSpacingRule
      = (lit("SPACING") >> (double_[boost::bind(&wrongDirParser, _1, sc, l)])
         >> lit("WRONGDIRECTION")
         >> -(lit("NONEOL")[boost::bind(
                  &odb::dbTechLayerWrongDirSpacingRule::setNoneolValid,
                  sc,
                  true)]
              >> double_[boost::bind(&noneolWidthParser, _1, sc, l)])
         >> -(lit("PRL") >> double_[boost::bind(&prlLengthParser, _1, sc, l)])
         >> -(lit("LENGTH")[boost::bind(
                  &odb::dbTechLayerWrongDirSpacingRule::setLengthValid,
                  sc,
                  true)]
              >> double_[boost::bind(&lengthParser, _1, sc, l)])
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, wrongDirSpacingRule, space)
               && first == last;

  if (!valid) {
    odb::dbTechLayerWrongDirSpacingRule::destroy(sc);
  }
  return valid;
}
}  // namespace odb::lefTechLayerWrongDirSpacing

namespace odb {

void lefTechLayerWrongDirSpacingParser::parse(std::string s,
                                              dbTechLayer* layer,
                                              odb::lefinReader* l)
{
  if (!lefTechLayerWrongDirSpacing::parse(s.begin(), s.end(), layer, l)) {
    l->warning(355,
               "parse mismatch in layer property LEF58_SPACING WRONGDIRECTION "
               "for layer {}",
               layer->getName());
  }
}

}  // namespace odb
