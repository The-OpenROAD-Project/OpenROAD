// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

void lefTechLayerMinStepParser::createSubRule(odb::dbTechLayer* layer)
{
  curRule = odb::dbTechLayerMinStepRule::create(layer);
}
void lefTechLayerMinStepParser::setMinAdjacentLength1(double length,
                                                      odb::lefinReader* l)
{
  curRule->setMinAdjLength1(l->dbdist(length));
  curRule->setMinAdjLength1Valid(true);
}
void lefTechLayerMinStepParser::setMinAdjacentLength2(double length,
                                                      odb::lefinReader* l)
{
  curRule->setMinAdjLength2(l->dbdist(length));
  curRule->setMinAdjLength2Valid(true);
}
void lefTechLayerMinStepParser::minBetweenLengthParser(double length,
                                                       odb::lefinReader* l)
{
  curRule->setMinBetweenLength(l->dbdist(length));
  curRule->setMinBetweenLengthValid(true);
}
void lefTechLayerMinStepParser::noBetweenEolParser(double width,
                                                   odb::lefinReader* l)
{
  curRule->setEolWidth(l->dbdist(width));
  curRule->setNoBetweenEol(true);
}

void lefTechLayerMinStepParser::noAdjacentEolParser(double width,
                                                    odb::lefinReader* l)
{
  curRule->setEolWidth(l->dbdist(width));
  curRule->setNoAdjacentEol(true);
}

void lefTechLayerMinStepParser::minStepLengthParser(double length,
                                                    odb::lefinReader* l)
{
  curRule->setMinStepLength(l->dbdist(length));
}
void lefTechLayerMinStepParser::maxEdgesParser(int edges, odb::lefinReader* l)
{
  curRule->setMaxEdges(edges);
  curRule->setMaxEdgesValid(true);
}

void lefTechLayerMinStepParser::setExceptRectangle()
{
  curRule->setExceptRectangle(true);
}

void lefTechLayerMinStepParser::setConvexCorner()
{
  curRule->setConvexCorner(true);
}

void lefTechLayerMinStepParser::setConcaveCorner()
{
  curRule->setConcaveCorner(true);
}

void lefTechLayerMinStepParser::setExceptSameCorners()
{
  curRule->setExceptSameCorners(true);
}

bool lefTechLayerMinStepParser::parse(std::string s,
                                      dbTechLayer* layer,
                                      odb::lefinReader* l)
{
  qi::rule<std::string::iterator, space_type> convexConcaveRule
      = (string("CONVEXCORNER")[boost::bind(
             &lefTechLayerMinStepParser::setConvexCorner, this)]
         | string("CONCAVECORNER")[boost::bind(
             &lefTechLayerMinStepParser::setConcaveCorner, this)]);
  qi::rule<std::string::iterator, space_type> minAdjacentRule
      = (lit("MINADJACENTLENGTH") >> double_[boost::bind(
             &lefTechLayerMinStepParser::setMinAdjacentLength1, this, _1, l)]
         >> -(convexConcaveRule
              | double_[boost::bind(
                  &lefTechLayerMinStepParser::setMinAdjacentLength2,
                  this,
                  _1,
                  l)]));

  qi::rule<std::string::iterator, space_type> minBetweenLengthRule
      = (lit("MINBETWEENLENGTH") >> (double_[boost::bind(
             &lefTechLayerMinStepParser::minBetweenLengthParser, this, _1, l)])
         >> -(lit("EXCEPTSAMECORNERS")[boost::bind(
             &lefTechLayerMinStepParser::setExceptSameCorners, this)]));
  qi::rule<std::string::iterator, space_type> noBetweenEolRule
      = (lit("NOBETWEENEOL") >> double_)[boost::bind(
          &lefTechLayerMinStepParser::noBetweenEolParser, this, _1, l)];
  qi::rule<std::string::iterator, space_type> noAdjacentEolRule
      = (lit("NOADJACENTEOL") >> double_)[boost::bind(
            &lefTechLayerMinStepParser::noAdjacentEolParser, this, _1, l)]
        >> -(lit("MINADJACENTLENGTH") >> double_[boost::bind(
                 &lefTechLayerMinStepParser::setMinAdjacentLength1,
                 this,
                 _1,
                 l)]);
  qi::rule<std::string::iterator, space_type> minstepRule
      = (+(lit("MINSTEP")[boost::bind(
               &lefTechLayerMinStepParser::createSubRule, this, layer)]
           >> double_[boost::bind(
               &lefTechLayerMinStepParser::minStepLengthParser, this, _1, l)]
           >> -(lit("MAXEDGES") >> int_[boost::bind(
                    &lefTechLayerMinStepParser::maxEdgesParser, this, _1, l)])
           >> -(lit("EXCEPTRECTANGLE")[boost::bind(
               &lefTechLayerMinStepParser::setExceptRectangle, this)])
           >> -(minAdjacentRule | minBetweenLengthRule | noAdjacentEolRule
                | noBetweenEolRule)
           >> lit(";")));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, minstepRule, space) && first == last;

  if (!valid && curRule != nullptr) {  // fail if we did not get a full match
    odb::dbTechLayerMinStepRule::destroy(curRule);
  }
  return valid;
}

}  // namespace odb
