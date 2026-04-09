// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCutSpacing {

void setCutSpacing(double value,
                   odb::lefTechLayerCutSpacingParser* parser,
                   odb::dbTechLayer* layer,
                   odb::lefinReader* lefinReader)
{
  parser->rule = odb::dbTechLayerCutSpacingRule::create(layer);
  parser->rule->setCutSpacing(lefinReader->dbdist(value));
  parser->rule->setType(odb::dbTechLayerCutSpacingRule::CutSpacingType::NONE);
}
void setBool(odb::lefTechLayerCutSpacingParser* parser,
             void (odb::dbTechLayerCutSpacingRule::*func)(bool),
             bool val)
{
  (parser->rule->*func)(val);
}
void setCenterToCenter(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setCenterToCenter(true);
}
void setSameMetal(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setSameMetal(true);
}
void setSameNet(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setSameNet(true);
}
void setSameVia(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setSameVia(true);
}
void addMaxXYSubRule(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setType(odb::dbTechLayerCutSpacingRule::CutSpacingType::MAXXY);
}
void addSameMaskSubRule(odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMASK);
}
void addLayerSubRule(
    const std::string& name,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  parser->rule->setType(odb::dbTechLayerCutSpacingRule::CutSpacingType::LAYER);
  auto second_layer = layer->getTech()->findLayer(name.c_str());
  if (second_layer != nullptr) {
    parser->rule->setSecondLayer(second_layer);
  } else {
    incomplete_props.emplace_back(parser->rule, name);
  }
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
    odb::lefTechLayerCutSpacingParser* parser,
    odb::dbTechLayer* layer,
    odb::lefinReader* lefinReader)
{
  parser->rule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS);
  // auto var = at_c<0>(params);
  auto cuts = at_c<0>(params);
  auto aligned = at_c<1>(params);
  auto two_cuts = at_c<2>(params);
  auto within = at_c<3>(params);
  auto within2 = at_c<4>(params);
  auto except_same_pgnet = at_c<5>(params);
  auto class_name = at_c<6>(params);
  auto side_parallel_no_prl = at_c<7>(params);
  auto same_mask = at_c<8>(params);
  uint32_t cuts_int = (uint32_t) cuts[0] - (uint32_t) '0';
  parser->rule->setAdjacentCuts(cuts_int);
  if (aligned.is_initialized()) {
    parser->rule->setExactAligned(true);
    parser->rule->setNumCuts(aligned.value());
  }
  if (two_cuts.is_initialized()) {
    parser->rule->setTwoCutsValid(true);
    parser->rule->setTwoCuts(two_cuts.value());
  }
  parser->rule->setWithin(lefinReader->dbdist(within));
  if (within2.is_initialized()) {
    parser->rule->setSecondWithin(lefinReader->dbdist(within2.value()));
  }
  if (except_same_pgnet.is_initialized()) {
    parser->rule->setExceptSamePgnet(true);
  }
  if (class_name.is_initialized()) {
    const auto& cut_class_name = class_name.value();
    auto cut_class = layer->findTechLayerCutClassRule(cut_class_name.c_str());
    if (cut_class != nullptr) {
      parser->rule->setCutClass(cut_class);
    }
  }
  if (side_parallel_no_prl.is_initialized()) {
    const auto& option = side_parallel_no_prl.value();
    if (option == "NOPRL") {
      parser->rule->setNoPrl(true);
    } else {
      parser->rule->setSideParallelOverlap(true);
    }
  }
  if (same_mask.is_initialized()) {
    parser->rule->setSameMask(true);
  }
}
void addParallelOverlapSubRule(boost::optional<std::string> except,
                               odb::lefTechLayerCutSpacingParser* parser)
{
  parser->rule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELOVERLAP);
  if (except.is_initialized()) {
    const auto& except_what = except.value();
    if (except_what == "EXCEPTSAMENET") {
      parser->rule->setExceptSameNet(true);
    } else if (except_what == "EXCEPTSAMEMETAL") {
      parser->rule->setExceptSameMetal(true);
    } else if (except_what == "EXCEPTSAMEVIA") {
      parser->rule->setExceptSameVia(true);
    } else if (except_what == "EXCEPTSAMEMETALOVERLAP") {
      parser->rule->setExceptSameMetalOverlap(true);
    }
  }
}
void addParallelWithinSubRule(
    boost::fusion::vector<double, boost::optional<std::string>>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::PARALLELWITHIN);
  parser->rule->setWithin(lefinReader->dbdist(at_c<0>(params)));
  auto except = at_c<1>(params);
  if (except.is_initialized()) {
    parser->rule->setExceptSameNet(true);
  }
}
void addSameMetalSharedEdgeSubRule(
    boost::fusion::vector<double,
                          boost::optional<std::string>,
                          boost::optional<std::string>,
                          boost::optional<std::string>,
                          boost::optional<int>>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::dbTechLayer* layer,
    odb::lefinReader* lefinReader)
{
  parser->rule->setType(
      odb::dbTechLayerCutSpacingRule::CutSpacingType::SAMEMETALSHAREDEDGE);
  auto within = at_c<0>(params);
  auto above = at_c<1>(params);
  auto cutclass = at_c<2>(params);
  auto except_two_edges = at_c<3>(params);
  auto except_same_via = at_c<4>(params);
  parser->rule->setWithin(lefinReader->dbdist(within));
  if (above.is_initialized()) {
    parser->rule->setAbove(true);
  }
  if (cutclass.is_initialized()) {
    const auto& cut_class_name = cutclass.value();
    auto cut_class = layer->findTechLayerCutClassRule(cut_class_name.c_str());
    if (cut_class != nullptr) {
      parser->rule->setCutClass(cut_class);
    }
  }
  if (except_two_edges.is_initialized()) {
    parser->rule->setExceptTwoEdges(true);
  }
  if (except_same_via.is_initialized()) {
    parser->rule->setExceptSameVia(true);
    auto num_cut = except_same_via.value();
    parser->rule->setNumCuts(num_cut);
  }
}
void addAreaSubRule(double value,
                    odb::lefTechLayerCutSpacingParser* parser,
                    odb::lefinReader* lefinReader)
{
  parser->rule->setType(odb::dbTechLayerCutSpacingRule::CutSpacingType::AREA);
  parser->rule->setCutArea(lefinReader->dbdist(value));
}

void setConcaveCornerWidth(
    boost::fusion::vector<double, double, double>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setConcaveCornerWidth(true);
  parser->rule->setWidth(lefinReader->dbdist(at_c<0>(params)));
  parser->rule->setEnclosure(lefinReader->dbdist(at_c<1>(params)));
  parser->rule->setEdgeLength(lefinReader->dbdist(at_c<2>(params)));
}

void setConcaveCornerParallel(
    boost::fusion::vector<double, double, double>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setConcaveCornerParallel(true);
  parser->rule->setParLength(lefinReader->dbdist(at_c<0>(params)));
  parser->rule->setParWithin(lefinReader->dbdist(at_c<1>(params)));
  parser->rule->setEnclosure(lefinReader->dbdist(at_c<2>(params)));
}
void setPrl(double value,
            odb::lefTechLayerCutSpacingParser* parser,
            odb::lefinReader* lefinReader)
{
  parser->rule->setPrlValid(true);
  parser->rule->setPrl(lefinReader->dbdist(value));
}

void setConcaveCornerEdgeLength(
    boost::fusion::vector<double, double, double>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::lefinReader* lefinReader)
{
  parser->rule->setConcaveCornerEdgeLength(true);
  parser->rule->setEdgeLength(lefinReader->dbdist(at_c<0>(params)));
  parser->rule->setEdgeEnclosure(lefinReader->dbdist(at_c<1>(params)));
  parser->rule->setAdjEnclosure(lefinReader->dbdist(at_c<2>(params)));
}

void setParWithinEnclosure(
    boost::fusion::vector<double, std::string, double, double>& params,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::lefinReader* lefinReader)
{
  auto above_below = at_c<1>(params);
  if (above_below == "ABOVE") {
    parser->rule->setAbove(true);
  } else {
    parser->rule->setBelow(true);
  }
  parser->rule->setParWithinEnclosureValid(true);
  parser->rule->setParEnclosure(lefinReader->dbdist(at_c<0>(params)));
  parser->rule->setParLength(lefinReader->dbdist(at_c<2>(params)));
  parser->rule->setParWithin(lefinReader->dbdist(at_c<3>(params)));
}

void setCutClassExtension(double value,
                          odb::lefTechLayerCutSpacingParser* parser,
                          odb::lefinReader* lefinReader)
{
  parser->rule->setExtensionValid(true);
  parser->rule->setExtension(lefinReader->dbdist(value));
}
void setCutClassNonEolConvexCorner(double value,
                                   odb::lefTechLayerCutSpacingParser* parser,
                                   odb::lefinReader* lefinReader)
{
  parser->rule->setNonEolConvexCorner(true);
  parser->rule->setEolWidth(lefinReader->dbdist(value));
}
void setMinLength(double value,
                  odb::lefTechLayerCutSpacingParser* parser,
                  odb::lefinReader* lefinReader)
{
  parser->rule->setMinLengthValid(true);
  parser->rule->setMinLength(lefinReader->dbdist(value));
}
void setAboveWidth(double value,
                   odb::lefTechLayerCutSpacingParser* parser,
                   odb::lefinReader* lefinReader)
{
  parser->rule->setAboveWidthValid(true);
  parser->rule->setAboveWidth(lefinReader->dbdist(value));
}
void setAboveWidthEnclosure(double value,
                            odb::lefTechLayerCutSpacingParser* parser,
                            odb::lefinReader* lefinReader)
{
  parser->rule->setAboveWidthEnclosureValid(true);
  parser->rule->setAboveEnclosure(lefinReader->dbdist(value));
}
void setOrthogonalSpacing(double value,
                          odb::lefTechLayerCutSpacingParser* parser,
                          odb::lefinReader* lefinReader)
{
  parser->rule->setOrthogonalSpacingValid(true);
  parser->rule->setOrthogonalSpacing(lefinReader->dbdist(value));
}
void setCutClass(const std::string& value,
                 odb::lefTechLayerCutSpacingParser* parser,
                 odb::dbTechLayer* layer)
{
  auto cut_class = layer->findTechLayerCutClassRule(value.c_str());
  if (cut_class != nullptr) {
    parser->rule->setCutClass(cut_class);
  }
}
template <typename Iterator>
bool parse(
    Iterator first,
    Iterator last,
    odb::lefTechLayerCutSpacingParser* parser,
    odb::dbTechLayer* layer,
    odb::lefinReader* lefinReader,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  qi::rule<std::string::const_iterator, space_type> layer_cut_class_rule
      = (lit("CUTCLASS")
         >> _string[boost::bind(&setCutClass, _1, parser, layer)] >> -(
             lit("SHORTEDGEONLY")[boost::bind(
                 &setBool,
                 parser,
                 &odb::dbTechLayerCutSpacingRule::setShortEdgeOnly,
                 true)]
                 >> -(lit("PRL")
                      >> double_)[boost::bind(&setPrl, _1, parser, lefinReader)]
             | lit("CONCAVECORNER")[boost::bind(
                   &setBool,
                   parser,
                   &odb::dbTechLayerCutSpacingRule::setConcaveCorner,
                   true)]
                   >> -((lit("WIDTH") >> double_ >> lit("ENCLOSURE") >> double_
                         >> lit("EDGELENGTH") >> double_)[boost::bind(
                            &setConcaveCornerWidth, _1, parser, lefinReader)]
                        | (lit("PARALLEL") >> double_ >> lit("WITHIN")
                           >> double_ >> lit("ENCLOSURE")
                           >> double_)[boost::bind(
                            &setConcaveCornerParallel, _1, parser, lefinReader)]
                        | (lit("EDGELENGTH") >> double_ >> lit("ENCLOSURE")
                           >> double_
                           >> double_)[boost::bind(&setConcaveCornerEdgeLength,
                                                   _1,
                                                   parser,
                                                   lefinReader)])
             | lit("EXTENSION") >> double_[boost::bind(
                   &setCutClassExtension, _1, parser, lefinReader)]
             | lit("NONEOLCONVEXCORNER") >> double_[boost::bind(
                   &setCutClassNonEolConvexCorner, _1, parser, lefinReader)]
                   >> -(lit("MINLENGTH") >> double_[boost::bind(
                            &setMinLength, _1, parser, lefinReader)])
             | lit("ABOVEWIDTH") >> double_[boost::bind(
                   &setAboveWidth, _1, parser, lefinReader)]
                   >> -(lit("ENCLOSURE") >> double_[boost::bind(
                            &setAboveWidthEnclosure, _1, parser, lefinReader)])
             | lit("MASKOVERLAP")[boost::bind(
                 &setBool,
                 parser,
                 &odb::dbTechLayerCutSpacingRule::setMaskOverlap,
                 true)]
             | lit("WRONGDIRECTION")[boost::bind(
                 &setBool,
                 parser,
                 &odb::dbTechLayerCutSpacingRule::setWrongDirection,
                 true)]));
  qi::rule<std::string::const_iterator, space_type> layer_rule
      = (lit("LAYER") >> _string[boost::bind(
             &addLayerSubRule, _1, parser, layer, boost::ref(incomplete_props))]
         >> -(
             lit("STACK")[boost::bind(&setBool,
                                      parser,
                                      &odb::dbTechLayerCutSpacingRule::setStack,
                                      true)]
             | lit("ORTHOGONALSPACING") >> double_[boost::bind(
                   &setOrthogonalSpacing, _1, parser, lefinReader)]
             | layer_cut_class_rule));

  qi::rule<std::string::const_iterator, space_type> adjacent_cuts_rule
      = (lit("ADJACENTCUTS") >> (string("1") | string("2") | string("3"))
         >> -(lit("EXACTALIGNED") >> int_)
         >> -(lit("TWOCUTS") >> int_ >> -lit("SAMECUT")[boost::bind(
                  &setBool,
                  parser,
                  &odb::dbTechLayerCutSpacingRule::setSameCut,
                  true)])
         >> lit("WITHIN") >> double_ >> -double_ >> -string("EXCEPTSAMEPGNET")
         >> -(lit("CUTCLASS") >> _string >> -lit("TO ALL")[boost::bind(
                  &setBool,
                  parser,
                  &odb::dbTechLayerCutSpacingRule::setCutClassToAll,
                  true)])
         >> -(string("SIDEPARALLELOVERLAP") | string("NOPRL"))
         >> -string("SAMEMASK"))[boost::bind(
          &addAdjacentCutsSubRule, _1, parser, layer, lefinReader)];

  qi::rule<std::string::const_iterator, space_type> parallel_overlap_rule
      = (lit("PARALLELOVERLAP")
         >> -(string("EXCEPTSAMENET") | string("EXCEPTSAMEMETAL")
              | string("EXCEPTSAMEVIA") | string("EXCEPTSAMEMETALOVERLAP")))
          [boost::bind(&addParallelOverlapSubRule, _1, parser)];
  qi::rule<std::string::const_iterator, space_type>
      parallel_within_cut_class_rule
      = (lit("CUTCLASS")
         >> _string[boost::bind(&setCutClass, _1, parser, layer)]
         >> -(lit("LONGEDGEONLY")[boost::bind(
                  &setBool,
                  parser,
                  &odb::dbTechLayerCutSpacingRule::setLongEdgeOnly,
                  true)]
              | (lit("ENCLOSURE") >> double_
                 >> (string("ABOVE") | string("BELOW")) >> lit("PARALLEL")
                 >> double_ >> lit("WITHIN") >> double_)[boost::bind(
                  &setParWithinEnclosure, _1, parser, lefinReader)]));

  qi::rule<std::string::const_iterator, space_type> parallel_within_rule
      = ((lit("PARALLELWITHIN") >> double_
          >> -string("EXCEPTSAMENET"))[boost::bind(
             &addParallelWithinSubRule, _1, parser, lefinReader)]
         >> -parallel_within_cut_class_rule);

  qi::rule<std::string::const_iterator, space_type> same_metal_shared_edge_rule
      = (lit("SAMEMETALSHAREDEDGE") >> double_ >> -string("ABOVE")
         >> -(lit("CUTCLASS") >> _string) >> -string("EXCEPTTWOEDGES")
         >> -(lit("EXCEPTSAMEVIA") >> int_))[boost::bind(
          &addSameMetalSharedEdgeSubRule, _1, parser, layer, lefinReader)];

  qi::rule<std::string::const_iterator, space_type> area_rule
      = (lit("AREA")
         >> double_)[boost::bind(&addAreaSubRule, _1, parser, lefinReader)];

  qi::rule<std::string::const_iterator, space_type> lef58_spacing_rule = (+(
      lit("SPACING")
      >> double_[boost::bind(&setCutSpacing, _1, parser, layer, lefinReader)]
      >> -(lit("MAXXY")[boost::bind(&addMaxXYSubRule, parser)]
           | lit("SAMEMASK")[boost::bind(&addSameMaskSubRule, parser)]
           | -lit("CENTERTOCENTER")[boost::bind(&setCenterToCenter, parser)]
                 >> -(lit("SAMENET")[boost::bind(&setSameNet, parser)]
                      | lit("SAMEMETAL")[boost::bind(&setSameMetal, parser)]
                      | lit("SAMEVIA")[boost::bind(&setSameVia, parser)])
                 >> -(layer_rule | adjacent_cuts_rule | parallel_overlap_rule
                      | parallel_within_rule | same_metal_shared_edge_rule
                      | area_rule))
      >> lit(";")));

  bool valid = qi::phrase_parse(first, last, lef58_spacing_rule, space)
               && first == last;

  if (!valid && parser->rule != nullptr) {
    if (!incomplete_props.empty()
        && incomplete_props.back().first == parser->rule) {
      incomplete_props.pop_back();
    }
    odb::dbTechLayerCutSpacingRule::destroy(parser->rule);
  }
  return valid;
}
}  // namespace odb::lefTechLayerCutSpacing

namespace odb {

bool lefTechLayerCutSpacingParser::parse(
    const std::string& s,
    odb::dbTechLayer* layer,
    odb::lefinReader* l,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  return lefTechLayerCutSpacing::parse(
      s.begin(), s.end(), this, layer, l, incomplete_props);
}

}  // namespace odb