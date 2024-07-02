/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace lefTechLayerWrongDirSpacing {

void wrongDirParser(double value,
                    odb::dbTechLayerWrongDirSpacingRule* sc,
                    odb::lefin* l)
{
  sc->setWrongdirSpace(l->dbdist(value));
}

void noneolWidthParser(double value,
                       odb::dbTechLayerWrongDirSpacingRule* sc,
                       odb::lefin* l)
{
  sc->setNoneolWidth(l->dbdist(value));
}

void prlLengthParser(double value,
                     odb::dbTechLayerWrongDirSpacingRule* sc,
                     odb::lefin* l)
{
  sc->setPrlLength(l->dbdist(value));
}

void lengthParser(double value,
                  odb::dbTechLayerWrongDirSpacingRule* sc,
                  odb::lefin* l)
{
  sc->setLength(l->dbdist(value));
}

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefin* l)
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
}  // namespace lefTechLayerWrongDirSpacing

namespace odb {

void lefTechLayerWrongDirSpacingParser::parse(std::string s,
                                              dbTechLayer* layer,
                                              odb::lefin* l)
{
  if (!lefTechLayerWrongDirSpacing::parse(s.begin(), s.end(), layer, l)) {
    l->warning(355,
               "parse mismatch in layer propery LEF58_SPACING WRONGDIRECTION "
               "for layer {}",
               layer->getName());
  }
}

}  // namespace odb
