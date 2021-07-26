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

#include "boostParser.h"
#include <string>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace odb {

void lefTechLayerMinStepParser::createSubRule(odb::dbTechLayer* layer)
{
  curRule = odb::dbTechLayerMinStepRule::create(layer);
}
void lefTechLayerMinStepParser::setMinAdjacentLength1(double length,
                                                      odb::lefin* l)
{
  curRule->setMinAdjLength1(l->dbdist(length));
  curRule->setMinAdjLength1Valid(true);
}
void lefTechLayerMinStepParser::setMinAdjacentLength2(double length,
                                                      odb::lefin* l)
{
  curRule->setMinAdjLength2(l->dbdist(length));
  curRule->setMinAdjLength2Valid(true);
}
void lefTechLayerMinStepParser::minBetweenLngthParser(double length,
                                                      odb::lefin* l)
{
  curRule->setMinBetweenLength(l->dbdist(length));
  curRule->setMinBetweenLengthValid(true);
}
void lefTechLayerMinStepParser::noBetweenEolParser(double width, odb::lefin* l)
{
  curRule->setEolWidth(l->dbdist(width));
  curRule->setNoBetweenEol(true);
}
void lefTechLayerMinStepParser::minStepLengthParser(double length,
                                                    odb::lefin* l)
{
  curRule->setMinStepLength(l->dbdist(length));
}
void lefTechLayerMinStepParser::maxEdgesParser(int edges, odb::lefin* l)
{
  curRule->setMaxEdges(edges);
  curRule->setMaxEdgesValid(true);
}

void lefTechLayerMinStepParser::setConvexCorner()
{
  curRule->setConvexCorner(true);
}

void lefTechLayerMinStepParser::setExceptSameCorners()
{
  curRule->setExceptSameCorners(true);
}

bool lefTechLayerMinStepParser::parse(std::string s,
                                      dbTechLayer* layer,
                                      odb::lefin* l)
{
  qi::rule<std::string::iterator, space_type> minAdjacentRule
      = (lit("MINADJACENTLENGTH") >> double_[boost::bind(
             &lefTechLayerMinStepParser::setMinAdjacentLength1, this, _1, l)]
         >> -(string("CONVEXCORNER")[boost::bind(
                  &lefTechLayerMinStepParser::setConvexCorner, this)]
              | double_[boost::bind(
                  &lefTechLayerMinStepParser::setMinAdjacentLength2,
                  this,
                  _1,
                  l)]));

  qi::rule<std::string::iterator, space_type> minBetweenLngthRule
      = (lit("MINBETWEENLENGTH") >> (double_[boost::bind(
             &lefTechLayerMinStepParser::minBetweenLngthParser, this, _1, l)])
         >> -(lit("EXCEPTSAMECORNERS")[boost::bind(
             &lefTechLayerMinStepParser::setExceptSameCorners, this)]));
  qi::rule<std::string::iterator, space_type> noBetweenEolRule
      = (lit("NOBETWEENEOL") >> double_)[boost::bind(
          &lefTechLayerMinStepParser::noBetweenEolParser, this, _1, l)];
  qi::rule<std::string::iterator, space_type> minstepRule
      = (+(lit("MINSTEP")[boost::bind(
               &lefTechLayerMinStepParser::createSubRule, this, layer)]
           >> double_[boost::bind(
               &lefTechLayerMinStepParser::minStepLengthParser, this, _1, l)]
           >> -(lit("MAXEDGES") >> int_[boost::bind(
                    &lefTechLayerMinStepParser::maxEdgesParser, this, _1, l)])
           >> -(minAdjacentRule | minBetweenLngthRule | noBetweenEolRule)
           >> lit(";")));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, minstepRule, space) && first == last;

  if (!valid && curRule != nullptr)  // fail if we did not get a full match
    odb::dbTechLayerMinStepRule::destroy(curRule);
  return valid;
}

}  // namespace odb
