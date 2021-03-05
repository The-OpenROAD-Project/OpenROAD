#include <boost/bind.hpp>
#include <functional>
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
using boost::spirit::ascii::alpha;
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
void setBool( odb::lefTechLayerCutSpacingParser* parser, void (odb::dbTechLayerCutSpacingRule::* func) (bool), bool val)
{
    (parser->curRule->*func)(val);
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
  parser->curRule->setSameVia(true);
}
void addMaxXYSubRule(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
}
void addSameMaskSubRule(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMASK);
}
void addLayerSubRule(
    std::string name,
    odb::lefTechLayerCutSpacingParser*                                parser,
    odb::dbTechLayer*                                                 layer,
    std::vector<std::pair<odb::dbObject*, std::string>>&              incomplete_props)
{
  parser->curRule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
  auto secondLayer = layer->getTech()->findLayer(name.c_str());
  if (secondLayer != nullptr)
    parser->curRule->setSecondLayer(secondLayer);
  else
    incomplete_props.push_back({parser->curRule, name});
}

void addAdjacentCutsSubRule(
    boost::fusion::vector<std::string,
                          boost::optional<int>,
                          boost::optional<int>,
                          double,
                          boost::optional<double>,
                          boost::optional<std::string>,
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
  auto twocuts             = at_c<2>(params);
  auto within              = at_c<3>(params);
  auto within2             = at_c<4>(params);
  auto except_same_pgnet   = at_c<5>(params);
  auto className           = at_c<6>(params);
  auto sideParallelNoPrl = at_c<7>(params);
  auto sameMask = at_c<8>(params);
  uint cuts_int            = (uint) cuts[0] - (uint) '0';
  parser->curRule->setAdjacentCuts(cuts_int);
  if (aligned.is_initialized()) {
    parser->curRule->setExactAligned(true);
    parser->curRule->setNumCuts(aligned.value());
  }
  if(twocuts.is_initialized())
  {
    parser->curRule->setTwoCutsValid(true);
    parser->curRule->setTwoCuts(twocuts.value());
  }
  parser->curRule->setWithin(lefin->dbdist(within));
  if(within2.is_initialized())
    parser->curRule->setSecondWithin(lefin->dbdist(within2.value()));
  if (except_same_pgnet.is_initialized())
    parser->curRule->setExceptSamePgnet(true);
  if (className.is_initialized()) {
    auto cutClassName = className.value();
    auto cutClass     = layer->findTechLayerCutClassRule(cutClassName.c_str());
    if (cutClass != nullptr)
      parser->curRule->setCutClass(cutClass);
  }
  if (sideParallelNoPrl.is_initialized())
  {
    auto option = sideParallelNoPrl.value();
    if(option == "NOPRL")
    {
      parser->curRule->setNoPrl(true);
    }else
    {
      parser->curRule->setSideParallelOverlap(true);
    }
  }
  if(sameMask.is_initialized())
    parser->curRule->setSameMask(true);
    
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
    else if (exceptWhat == "EXCEPTSAMEMETALOVERLAP")
      parser->curRule->setExceptSameMetalOverlap(true);
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

void setConcaveCornerWidth(boost::fusion::vector< double, double, double>& params,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  parser->curRule->setConcaveCornerWidth(true);
  parser->curRule->setWidth(lefin->dbdist(at_c<0>(params)));
  parser->curRule->setEnclosure(lefin->dbdist(at_c<1>(params)));
  parser->curRule->setEdgeLength(lefin->dbdist(at_c<2>(params)));
}

void setConcaveCornerParallel(boost::fusion::vector< double, double, double>& params,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  parser->curRule->setConcaveCornerParallel(true);
  parser->curRule->setParLength(lefin->dbdist(at_c<0>(params)));
  parser->curRule->setParWithin(lefin->dbdist(at_c<1>(params)));
  parser->curRule->setEnclosure(lefin->dbdist(at_c<2>(params)));
}
void setPrl(double value,
            odb::lefTechLayerCutSpacingParser* parser,
            odb::lefin* lefin)
{
  parser->curRule->setPrlValid(true);
  parser->curRule->setPrl(lefin->dbdist(value));
}

void setConcaveCornerEdgeLength(boost::fusion::vector< double, double, double>& params,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  parser->curRule->setConcaveCornerEdgeLength(true);
  parser->curRule->setEdgeLength(lefin->dbdist(at_c<0>(params)));
  parser->curRule->setEdgeEnclosure(lefin->dbdist(at_c<1>(params)));
  parser->curRule->setAdjEnclosure(lefin->dbdist(at_c<2>(params)));
}

void setParWithinEnclosure(boost::fusion::vector< double, std::string, double, double>& params,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  auto aboveBelow = at_c<1>(params);
  if(aboveBelow=="ABOVE")
    parser->curRule->setAbove(true);
  else
    parser->curRule->setBelow(true);
  parser->curRule->setParWithinEnclosureValid(true);
  parser->curRule->setParEnclosure(lefin->dbdist(at_c<0>(params)));
  parser->curRule->setParLength(lefin->dbdist(at_c<2>(params)));
  parser->curRule->setParWithin(lefin->dbdist(at_c<3>(params)));
}

void setCutClassExtension( double value,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  parser->curRule->setExtensionValid(true);
  parser->curRule->setExtension(lefin->dbdist(value));
}
void setCutClassNonEolConvexCorner( double value,
                           odb::lefTechLayerCutSpacingParser* parser,
                           odb::lefin* lefin)
{
  parser->curRule->setNonEolConvexCorner(true);
  parser->curRule->setEolWidth(lefin->dbdist(value));
}
void setMinLength(double value, odb::lefTechLayerCutSpacingParser* parser, odb::lefin* lefin)
{
  parser->curRule->setMinLengthValid(true);
  parser->curRule->setMinLength(lefin->dbdist(value));
}
void setAboveWidth(double value, odb::lefTechLayerCutSpacingParser* parser, odb::lefin* lefin)
{
  parser->curRule->setAboveWidthValid(true);
  parser->curRule->setAboveWidth(lefin->dbdist(value));
}
void setAboveWidthEnclosure(double value, odb::lefTechLayerCutSpacingParser* parser, odb::lefin* lefin)
{
  parser->curRule->setAboveWidthEnclosureValid(true);
  parser->curRule->setAboveEnclosure(lefin->dbdist(value));
}
void setOrthogonalSpacing(double value, odb::lefTechLayerCutSpacingParser* parser, odb::lefin* lefin)
{
  parser->curRule->setOrthogonalSpacingValid(true);
  parser->curRule->setOrthogonalSpacingValid(lefin->dbdist(value));
}
void setCutClass(std::string value, odb::lefTechLayerCutSpacingParser* parser, odb::dbTechLayer* layer)
{
  auto cutClass     = layer->findTechLayerCutClassRule(value.c_str());
  if(cutClass != nullptr)
    parser->curRule->setCutClass(cutClass);
}
template <typename Iterator>
bool parse(Iterator                           first,
           Iterator                           last,
           odb::lefTechLayerCutSpacingParser* parser,
           odb::dbTechLayer*                  layer,
           odb::lefin*                        lefin,
           std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  qi::rule<Iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[(alpha >> *(char_ - ' ' - '\n'))];
  qi::rule<std::string::iterator, space_type> LAYER_CUTCLASS = (
    lit("CUTCLASS") 
    >> _string [boost::bind(&setCutClass, _1, parser, layer)]
    >> -(
      lit("SHORTEDGEONLY")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setShortEdgeOnly, true)]
      >> -(lit("PRL") >> double_)[boost::bind(&setPrl, _1, parser, lefin)]
      |
      lit("CONCAVECORNER")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setConcaveCorner, true)] 
      >> -(
        (lit ("WIDTH") >> double_ >> lit("ENCLOSURE") >> double_ >> lit("EDGELENGTH") >> double_)
        [boost::bind(&setConcaveCornerWidth,_1, parser, lefin)]
        |
        (lit ("PARALLEL") >> double_ >> lit("WITHIN") >> double_ >> lit("ENCLOSURE") >> double_)
        [boost::bind(&setConcaveCornerParallel,_1, parser, lefin)]
        |
        (lit ("EDGELENGTH") >> double_ >> lit("ENCLOSURE") >> double_ >> double_)
        [boost::bind(&setConcaveCornerEdgeLength,_1, parser, lefin)]
      )
      |
      lit("EXTENSION") >> double_ [boost::bind(&setCutClassExtension,_1, parser, lefin)]
      |
      lit("NONEOLCONVEXCORNER") >> double_[boost::bind(&setCutClassNonEolConvexCorner,_1, parser, lefin)] >> -(
        lit("MINLENGTH") >> double_[boost::bind(&setMinLength,_1, parser, lefin)]
        )
      |
      lit("ABOVEWIDTH") >> double_[boost::bind(&setAboveWidth,_1, parser, lefin)] >> -(
        lit("ENCLOSURE") >> double_[boost::bind(&setAboveWidthEnclosure,_1, parser, lefin)]
        )
      |
      lit("MASKOVERLAP")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setMaskOverlap, true)]
      |
      lit("WRONGDIRECTION")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setWrongDirection, true)]
    )
  );
  qi::rule<std::string::iterator, space_type> LAYER
      = (
        lit("LAYER") >> _string[boost::bind(&addLayerSubRule, _1, parser, layer, boost::ref(incomplete_props))]
         >> -(
           lit("STACK")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setStack, true)]
           |
           lit("ORTHOGONALSPACING") >> double_ [boost::bind(&setOrthogonalSpacing, _1, parser, lefin)]
           |
           LAYER_CUTCLASS
           )
      );

  qi::rule<std::string::iterator, space_type> ADJACENTCUTS
      = (lit("ADJACENTCUTS") >> (string("1") | string("2") | string("3"))
         >> -(lit("EXACTALIGNED") >> int_)
         >> -(lit("TWOCUTS") >> int_>> -lit("SAMECUT")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setSameCut, true)] )
         >> lit("WITHIN") >> double_ >> -double_
         >> -string("EXCEPTSAMEPGNET") >> -(lit("CUTCLASS") >> _string >> -lit("TO ALL")[boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setCutClassToAll, true)] )
         >> -(
           string("SIDEPARALLELOVERLAP")
           |
           string("NOPRL")
          )
         >> -string("SAMEMASK")
        )[boost::bind(
          &addAdjacentCutsSubRule, _1, parser, layer, lefin)];

  qi::rule<std::string::iterator, space_type> PARALLELOVERLAP
      = (lit("PARALLELOVERLAP")
         >> -(string("EXCEPTSAMENET") | string("EXCEPTSAMEMETAL")
              | string("EXCEPTSAMEVIA") | string("EXCEPTSAMEMETALOVERLAP")))
          [boost::bind(&addParallelOverlapSubRule, _1, parser)];
  qi::rule<std::string::iterator, space_type> PARALLELWITHIN_CUTCLASS = (
    lit("CUTCLASS") >> _string [boost::bind(&setCutClass, _1, parser, layer)]
    >> -(
      lit("LONGEDGEONLY") [boost::bind(&setBool, parser, &odb::dbTechLayerCutSpacingRule::setLongEdgeOnly, true)]
      |
      (lit("ENCLOSURE") >> double_ >> (string("ABOVE") | string("BELOW"))
      >> lit ("PARALLEL") >> double_ >> lit("WITHIN") >> double_)[boost::bind(&setParWithinEnclosure, _1, parser, lefin)]
    )
  );

  qi::rule<std::string::iterator, space_type> PARALLELWITHIN
      = (
        (lit("PARALLELWITHIN") >> double_ >> -string("EXCEPTSAMENET"))[boost::bind(&addParallelWithinSubRule, _1, parser, lefin)]
         >> -PARALLELWITHIN_CUTCLASS
         );

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
           | lit("SAMEMASK")[boost::bind(&addSameMaskSubRule, parser)]
           | -lit("CENTERTOCENTER")[boost::bind(&setCenterToCenter, parser)]
                 >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
                      | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
                      | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
                 >> -(LAYER | ADJACENTCUTS | PARALLELOVERLAP | PARALLELWITHIN | SAMEMETALSHAREDEDGE | AREA))
      >> lit(";")));

  bool valid = qi::phrase_parse(first, last, LEF58_SPACING, space);

  if (!valid && parser->curRule != nullptr){
    if(!incomplete_props.empty() && incomplete_props.back().first == parser->curRule)
      incomplete_props.pop_back();
    odb::dbTechLayerCutSpacingRule::destroy(parser->curRule);
  }
  return valid && first == last;
}
}  // namespace lefTechLayerCutSpacing

namespace odb {

bool lefTechLayerCutSpacingParser::parse(std::string       s,
                                         odb::dbTechLayer* layer,
                                         odb::lefin*       l,
                                         std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  return lefTechLayerCutSpacing::parse(s.begin(), s.end(), this, layer, l, incomplete_props);
}

}  // namespace odb
