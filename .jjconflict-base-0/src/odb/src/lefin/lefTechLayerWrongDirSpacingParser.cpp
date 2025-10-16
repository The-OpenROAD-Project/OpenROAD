// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Parser for LEF58 wrong-direction spacing rules that define spacing
// requirements for non-preferred direction shapes
#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerWrongDirSpacing {

// Set the base wrong-direction spacing value (converts to database units)
void wrongDirParser(double value,
                    odb::dbTechLayerWrongDirSpacingRule* sc,
                    odb::lefinReader* l)
{
  sc->setWrongdirSpace(l->dbdist(value));
}

// Set the non-end-of-line width parameter
void noneolWidthParser(double value,
                       odb::dbTechLayerWrongDirSpacingRule* sc,
                       odb::lefinReader* l)
{
  sc->setNoneolWidth(l->dbdist(value));
}

// Set the parallel run length parameter
void prlLengthParser(double value,
                     odb::dbTechLayerWrongDirSpacingRule* sc,
                     odb::lefinReader* l)
{
  sc->setPrlLength(l->dbdist(value));
}

// Set the length parameter
void lengthParser(double value,
                  odb::dbTechLayerWrongDirSpacingRule* sc,
                  odb::lefinReader* l)
{
  sc->setLength(l->dbdist(value));
}

// Parse a single wrong-direction spacing rule
// Format: SPACING value WRONGDIRECTION [NONEOL width] [PRL length] [LENGTH
// length] ;
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* l)
{
  odb::dbTechLayerWrongDirSpacingRule* sc
      = odb::dbTechLayerWrongDirSpacingRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> wrongDirSpacingRule
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

// Parse input string containing wrong-direction spacing rules for a layer
void lefTechLayerWrongDirSpacingParser::parse(const std::string& s,
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
