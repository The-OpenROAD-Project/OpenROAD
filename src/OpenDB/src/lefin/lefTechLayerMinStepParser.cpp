#include <boost/bind.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <string>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace odb {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

using boost::fusion::at_c;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;

using qi::double_;
using qi::int_;
// using qi::_1;
using ascii::space;
using phoenix::ref;
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
  bool valid = qi::phrase_parse(first, last, minstepRule, space);

  if (!valid && curRule != nullptr)  // fail if we did not get a full match
    odb::dbTechLayerMinStepRule::destroy(curRule);
  return valid && first == last;
}

}  // namespace odb
