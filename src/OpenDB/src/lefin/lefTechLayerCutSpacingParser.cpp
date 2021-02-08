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

namespace lefTechLayerCutSpacing {
namespace qi      = boost::spirit::qi;
namespace ascii   = boost::spirit::ascii;
namespace phoenix = boost::phoenix;
using ascii::char_;
using boost::fusion::at_c;
using boost::spirit::ascii::space_type;
using boost::spirit::ascii::string;
using boost::spirit::qi::lit;
using qi::lexeme;

using qi::double_;
using qi::int_;
// using qi::_1;
using ascii::space;
using phoenix::ref;
void setCutSpacing(double                             value,
                   odb::lefTechLayerCutSpacingParser* parser,
                   odb::dbTechLayer*                  layer,
                   odb::lefin*                        lefin)
{
  parser->curRule = odb::dbTechLayerCutSpacingRule::create(layer);
  parser->curRule->setCutSpacing(lefin->dbdist(value));
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::NONE);
}
void setCenterToCenter(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setCenterToCenter(true);
}
void setSameMetal(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setSameMetal(true);
}
void setSameNet(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setSameNet(true);
}
void setSameVia(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setSameVias(true);
}
void addMaxXYSubRule(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
}
void addLayerSubRule(
    boost::fusion::vector<std::string, boost::optional<std::string>>& params,
    odb::lefTechLayerCutSpacingParser*                                parser,
    odb::dbTechLayer*                                                 layer)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
  auto name        = at_c<0>(params);
  auto secondLayer = layer->getTech()->findLayer(name.c_str());
  if (secondLayer != nullptr)
    parser->curRule->setSecondLayer(secondLayer);
  else
    return;
  auto stack = at_c<1>(params);
  if (stack.is_initialized())
    parser->curRule->setStack(true);
}

void addAdjacentCutsSubRule(
    boost::fusion::vector<std::string,
                          boost::optional<int>,
                          double,
                          boost::optional<std::string>,
                          boost::optional<std::string>,
                          boost::optional<std::string>>& params,
    odb::lefTechLayerCutSpacingParser*                   parser,
    odb::dbTechLayer*                                    layer,
    odb::lefin*                                          lefin)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
  // auto var = at_c<0>(params);
  auto cuts                = at_c<0>(params);
  auto aligned             = at_c<1>(params);
  auto within              = at_c<2>(params);
  auto except_same_pgnet   = at_c<3>(params);
  auto className           = at_c<4>(params);
  auto sideParallelOverLap = at_c<5>(params);
  uint cuts_int            = (uint) cuts[0] - (uint) '0';
  parser->curRule->setAdjacentCuts(cuts_int);
  if (aligned.is_initialized()) {
    parser->curRule->setExactAligned(true);
    parser->curRule->setNumCuts(aligned.value());
  }
  parser->curRule->setWithin(lefin->dbdist(within));
  if (except_same_pgnet.is_initialized())
    parser->curRule->setExceptSamePgnet(true);
  if (className.is_initialized()) {
    auto cutClassName = className.value();
    auto cutClass     = layer->findTechLayerCutClassRule(cutClassName.c_str());
    if (cutClass != nullptr)
      parser->curRule->setCutClass(cutClass);
  }
  if (sideParallelOverLap.is_initialized())
    parser->curRule->setSideParallelOverlap(true);
}
void addParallelOverlapSubRule(boost::optional<std::string>       except,
                               odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELOVERLAP);
  if (except.is_initialized()) {
    auto exceptWhat = except.value();
    if (exceptWhat == "EXCEPTSAMENET")
      parser->curRule->setExceptSameNet(true);
    else if (exceptWhat == "EXCEPTSAMEMETAL")
      parser->curRule->setExceptSameMetal(true);
    else if (exceptWhat == "EXCEPTSAMEVIA")
      parser->curRule->setExceptSameVia(true);
  }
}
void addParallelWithinSubRule(
    boost::fusion::vector<double, boost::optional<std::string>>& params,
    odb::lefTechLayerCutSpacingParser*                           parser,
    odb::lefin*                                                  lefin)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELWITHIN);
  parser->curRule->setWithin(lefin->dbdist(at_c<0>(params)));
  auto except = at_c<1>(params);
  if (except.is_initialized())
    parser->curRule->setExceptSameNet(true);
}
void addSameMetalSharedEdgeSubRule(
    boost::fusion::vector<double,
                          boost::optional<std::string>,
                          boost::optional<std::string>,
                          boost::optional<std::string>,
                          boost::optional<int>>& params,
    odb::lefTechLayerCutSpacingParser*           parser,
    odb::dbTechLayer*                            layer,
    odb::lefin*                                  lefin)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMETALSHAREDEDGE);
  auto within         = at_c<0>(params);
  auto ABOVE          = at_c<1>(params);
  auto CUTCLASS       = at_c<2>(params);
  auto EXCEPTTWOEDGES = at_c<3>(params);
  auto EXCEPTSAMEVIA  = at_c<4>(params);
  parser->curRule->setWithin(lefin->dbdist(within));
  if (ABOVE.is_initialized())
    parser->curRule->setAbove(true);
  if (CUTCLASS.is_initialized()) {
    auto cutClassName = CUTCLASS.value();
    auto cutClass     = layer->findTechLayerCutClassRule(cutClassName.c_str());
    if (cutClass != nullptr)
      parser->curRule->setCutClass(cutClass);
  }
  if (EXCEPTTWOEDGES.is_initialized())
    parser->curRule->setExceptTwoEdges(true);
  if (EXCEPTSAMEVIA.is_initialized()) {
    parser->curRule->setExceptSameVia(true);
    auto numCut = EXCEPTSAMEVIA.value();
    parser->curRule->setNumCuts(numCut);
  }
}
void addAreaSubRule(double                             value,
                    odb::lefTechLayerCutSpacingParser* parser,
                    odb::lefin*                        lefin)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::AREA);
  parser->curRule->setCutArea(lefin->dbdist(value));
}

template <typename Iterator>
bool parse(Iterator                           first,
           Iterator                           last,
           odb::lefTechLayerCutSpacingParser* parser,
           odb::dbTechLayer*                  layer,
           odb::lefin*                        lefin)
{
  qi::rule<Iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  qi::rule<std::string::iterator, space_type> LAYER
      = (lit("LAYER") >> _string
         >> -string("STACK"))[boost::bind(&addLayerSubRule, _1, parser, layer)];

  qi::rule<std::string::iterator, space_type> ADJACENTCUTS
      = (lit("ADJACENTCUTS") >> (string("1") | string("2") | string("3"))
         >> -(lit("EXACTALIGNED") >> int_) >> lit("WITHIN") >> double_
         >> -string("EXCEPTSAMEPGNET") >> -(lit("CUTCLASS") >> _string)
         >> -string("SIDEPARALLELOVERLAP"))[boost::bind(
          &addAdjacentCutsSubRule, _1, parser, layer, lefin)];

  qi::rule<std::string::iterator, space_type> PARALLELOVERLAP
      = (lit("PARALLELOVERLAP")
         >> -(string("EXCEPTSAMENET") | string("EXCEPTSAMEMETAL")
              | string("EXCEPTSAMEVIA")))
          [boost::bind(&addParallelOverlapSubRule, _1, parser)];

  qi::rule<std::string::iterator, space_type> PARALLELWITHIN
      = (lit("PARALLELWITHIN") >> double_
         >> -string("EXCEPTSAMENET"))[boost::bind(
          &addParallelWithinSubRule, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> SAMEMETALSHAREDEDGE
      = (lit("SAMEMETALSHAREDEDGE") >> double_ >> -string("ABOVE")
         >> -(lit("CUTCLASS") >> _string) >> -string("EXCEPTTWOEDGES")
         >> -(lit("EXCEPTSAMEVIA") >> int_))[boost::bind(
          &addSameMetalSharedEdgeSubRule, _1, parser, layer, lefin)];

  qi::rule<std::string::iterator, space_type> AREA
      = (lit("AREA")
         >> double_)[boost::bind(&addAreaSubRule, _1, parser, lefin)];

  qi::rule<std::string::iterator, space_type> LEF58_SPACING = (+(
      lit("SPACING")
      >> double_[boost::bind(&setCutSpacing, _1, parser, layer, lefin)]
      >> -(lit("MAXXY")[boost::bind(&addMaxXYSubRule, parser)]
           | -lit("CENTERTOCENTER")[boost::bind(&setCenterToCenter, parser)]
                 >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
                      | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
                      | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
                 >> -(LAYER))
      >> lit(";")));

  bool valid = qi::phrase_parse(first, last, LEF58_SPACING, space);

  if (!valid && parser->curRule != nullptr)
    odb::dbTechLayerCutSpacingRule::destroy(parser->curRule);
  return valid && first == last;
}
}  // namespace lefTechLayerCutSpacing

namespace odb {

bool lefTechLayerCutSpacingParser::parse(std::string       s,
                                         odb::dbTechLayer* layer,
                                         odb::lefin*       l)
{
  return lefTechLayerCutSpacing::parse(s.begin(), s.end(), this, layer, l);
}

}  // namespace odb
