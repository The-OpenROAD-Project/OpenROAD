// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Parser for LEF58 area rules that define minimum area requirements for shapes
#include <string>
#include <utility>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

// Initialize parser with LEF reader
lefTechLayerAreaRuleParser::lefTechLayerAreaRuleParser(lefinReader* l)
{
  lefin_ = l;
}

// Parse input string containing area rules for a layer
void lefTechLayerAreaRuleParser::parse(
    const std::string& s,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  processRules(s, [this, layer, &incomplete_props](const std::string& rule) {
    if (!parseSubRule(rule, layer, incomplete_props)) {
      lefin_->warning(278,
                      "parse mismatch in layer property LEF58_AREA for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
    }
  });
}

// Helper function to set integer values (converts to database units)
void lefTechLayerAreaRuleParser::setInt(
    double val,
    odb::dbTechLayerAreaRule* rule,
    void (odb::dbTechLayerAreaRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}

// Set edge length exception parameters
void lefTechLayerAreaRuleParser::setExceptEdgeLengths(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptEdgeLengths(size);
}

// Set minimum size exception parameters
void lefTechLayerAreaRuleParser::setExceptMinSize(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptMinSize(size);
}

// Set step size exception parameters
void lefTechLayerAreaRuleParser::setExceptStep(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptStep(size);
}

// Set trim layer reference, handling incomplete properties
void lefTechLayerAreaRuleParser::setTrimLayer(
    const std::string& val,
    odb::dbTechLayerAreaRule* rule,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  auto trim_layer = layer->getTech()->findLayer(val.c_str());
  if (trim_layer != nullptr) {
    rule->setTrimLayer(trim_layer);
  } else {
    incomplete_props.emplace_back(rule, val);
  }
}

// Parse a single area rule
// Format: AREA value [MASK mask] [EXCEPTMINWIDTH width] [EXCEPTEDGELENGTH
// length1 length2]
//         [EXCEPTMINSIZE width height] [EXCEPTSTEP stepx stepy] [RECTWIDTH
//         width] [EXCEPTRECTANGLE] [LAYER name OVERLAP overlap] ;
bool lefTechLayerAreaRuleParser::parseSubRule(
    const std::string& s,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  odb::dbTechLayerAreaRule* rule = odb::dbTechLayerAreaRule::create(layer);

  qi::rule<std::string::const_iterator, space_type> EXCEPT_EDGE_LENGTH
      = ((lit("EXCEPTEDGELENGTH") >> double_ >> double_)[boost::bind(
             &lefTechLayerAreaRuleParser::setExceptEdgeLengths, this, _1, rule)]
         | lit("EXCEPTEDGELENGTH") >> double_[boost::bind(
               &lefTechLayerAreaRuleParser::setInt,
               this,
               _1,
               rule,
               &dbTechLayerAreaRule::setExceptEdgeLength)]);

  qi::rule<std::string::const_iterator, space_type> EXCEPT_MIN_SIZE
      = (lit("EXCEPTMINSIZE") >> double_ >> double_)[boost::bind(
          &lefTechLayerAreaRuleParser::setExceptMinSize, this, _1, rule)];

  qi::rule<std::string::const_iterator, space_type> EXCEPT_STEP
      = (lit("EXCEPTSTEP") >> double_ >> double_)[boost::bind(
          &lefTechLayerAreaRuleParser::setExceptStep, this, _1, rule)];

  qi::rule<std::string::const_iterator, space_type> LAYER
      = ((lit("LAYER")
          >> _string[boost::bind(&lefTechLayerAreaRuleParser::setTrimLayer,
                                 this,
                                 _1,
                                 rule,
                                 layer,
                                 boost::ref(incomplete_props))])
         >> lit("OVERLAP")
         >> int_[boost::bind(&odb::dbTechLayerAreaRule::setOverlap, rule, _1)]);

  qi::rule<std::string::const_iterator, space_type> AREA
      = (lit("AREA") >> double_[boost::bind(&lefTechLayerAreaRuleParser::setInt,
                                            this,
                                            _1,
                                            rule,
                                            &odb::dbTechLayerAreaRule::setArea)]
         >> -(
             lit("MASK")
             >> int_[boost::bind(&odb::dbTechLayerAreaRule::setMask, rule, _1)])
         >> -(lit("EXCEPTMINWIDTH") >> double_[boost::bind(
                  &lefTechLayerAreaRuleParser::setInt,
                  this,
                  _1,
                  rule,
                  &odb::dbTechLayerAreaRule::setExceptMinWidth)])
         >> -EXCEPT_EDGE_LENGTH >> -EXCEPT_MIN_SIZE >> -EXCEPT_STEP
         >> -(lit("RECTWIDTH")
              >> double_[boost::bind(&lefTechLayerAreaRuleParser::setInt,
                                     this,
                                     _1,
                                     rule,
                                     &odb::dbTechLayerAreaRule::setRectWidth)])
         >> -(lit("EXCEPTRECTANGLE")[boost::bind(
             &dbTechLayerAreaRule::setExceptRectangle, rule, true)])
         >> -LAYER >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, AREA, space) && first == last;
  if (!valid) {
    if (!incomplete_props.empty() && incomplete_props.back().first == rule) {
      incomplete_props.pop_back();
    }
    odb::dbTechLayerAreaRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
