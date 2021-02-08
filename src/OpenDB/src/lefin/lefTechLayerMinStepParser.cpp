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
#include <iostream>
#include <string>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace lefTechLayerMinStep {
namespace qi      = boost::spirit::qi;
namespace ascii   = boost::spirit::ascii;
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

void createSubRule(odb::lefTechLayerMinStepParser* parser,
                   odb::dbTechLayer*               layer)
{
  parser->curRule = odb::dbTechLayerMinStepRule::create(layer);
}
void setMinAdjacentLength1(double                          length,
                           odb::lefTechLayerMinStepParser* parser,
                           odb::lefin*                     l)
{
  parser->curRule->setMinAdjLength1(l->dbdist(length));
  parser->curRule->setMinAdjLength1Valid(true);
}
void setMinAdjacentLength2(double                          length,
                           odb::lefTechLayerMinStepParser* parser,
                           odb::lefin*                     l)
{
  parser->curRule->setMinAdjLength2(l->dbdist(length));
  parser->curRule->setMinAdjLength2Valid(true);
}
void minBetweenLngthParser(double                          length,
                           odb::lefTechLayerMinStepParser* parser,
                           odb::lefin*                     l)
{
  parser->curRule->setMinBetweenLength(l->dbdist(length));
  parser->curRule->setMinBetweenLengthValid(true);
}
void minStepLengthParser(double                          length,
                         odb::lefTechLayerMinStepParser* parser,
                         odb::lefin*                     l)
{
  parser->curRule->setMinStepLength(l->dbdist(length));
}
void maxEdgesParser(int                             edges,
                    odb::lefTechLayerMinStepParser* parser,
                    odb::lefin*                     l)
{
  parser->curRule->setMaxEdges(edges);
  parser->curRule->setMaxEdgesValid(true);
}

void setConvexCorner(odb::lefTechLayerMinStepParser* parser)
{
  parser->curRule->setConvexCorner(true);
}

void setExceptSameCorners(odb::lefTechLayerMinStepParser* parser)
{
  parser->curRule->setExceptSameCorners(true);
}

template <typename Iterator>
bool parse(Iterator                        first,
           Iterator                        last,
           odb::lefTechLayerMinStepParser* parser,
           odb::dbTechLayer*               layer,
           odb::lefin*                     l)
{
  qi::rule<std::string::iterator, space_type> minAdjacentRule
      = (lit("MINADJACENTLENGTH")
         >> double_[boost::bind(&setMinAdjacentLength1, _1, parser, l)]
         >> -(string("CONVEXCORNER")[boost::bind(&setConvexCorner, parser)]
              | double_[boost::bind(&setMinAdjacentLength2, _1, parser, l)]));

  qi::rule<std::string::iterator, space_type> minBetweenLngthRule
      = (lit("MINBETWEENLENGTH")
         >> (double_[boost::bind(&minBetweenLngthParser, _1, parser, l)])
         >> -(lit(
             "EXCEPTSAMECORNERS")[boost::bind(&setExceptSameCorners, parser)]));
  qi::rule<std::string::iterator, space_type> minstepRule = (+(
      lit("MINSTEP")[boost::bind(&createSubRule, parser, layer)]
      >> double_[boost::bind(&minStepLengthParser, _1, parser, l)]
      >> -(lit("MAXEDGES") >> int_[boost::bind(&maxEdgesParser, _1, parser, l)])
      >> -(minAdjacentRule | minBetweenLngthRule) >> lit(";")));

  bool valid = qi::phrase_parse(first, last, minstepRule, space);

  if (!valid
      && parser->curRule != nullptr)  // fail if we did not get a full match
    odb::dbTechLayerMinStepRule::destroy(parser->curRule);
  return valid && first == last;
}
}  // namespace lefTechLayerMinStep

namespace odb {

bool lefTechLayerMinStepParser::parse(std::string  s,
                                      dbTechLayer* layer,
                                      odb::lefin*  l)
{
  return lefTechLayerMinStep::parse(s.begin(), s.end(), this, layer, l);
}

}  // namespace odb
