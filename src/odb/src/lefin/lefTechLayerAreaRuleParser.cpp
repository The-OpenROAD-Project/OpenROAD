/*
 * Copyright (c) 2022, The Regents of the University of California
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

#include <functional>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerAreaRuleParser::lefTechLayerAreaRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerAreaRuleParser::parse(
    const std::string& s,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto& rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty()) {
      continue;
    }
    rule += " ; ";
    if (!parseSubRule(rule, layer, incomplete_props)) {
      lefin_->warning(278,
                      "parse mismatch in layer propery LEF58_AREA for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
    }
  }
}

void lefTechLayerAreaRuleParser::setInt(
    double val,
    odb::dbTechLayerAreaRule* rule,
    void (odb::dbTechLayerAreaRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}

void lefTechLayerAreaRuleParser::setExceptEdgeLengths(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptEdgeLengths(size);
}

void lefTechLayerAreaRuleParser::setExceptMinSize(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptMinSize(size);
}

void lefTechLayerAreaRuleParser::setExceptStep(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerAreaRule* rule)
{
  std::pair<int, int> size(lefin_->dbdist(at_c<0>(params)),
                           lefin_->dbdist(at_c<1>(params)));
  rule->setExceptStep(size);
}

void lefTechLayerAreaRuleParser::setTrimLayer(
    std::string val,
    odb::dbTechLayerAreaRule* rule,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  auto trim_layer = layer->getTech()->findLayer(val.c_str());
  if (trim_layer != nullptr) {
    rule->setTrimLayer(trim_layer);
  } else {
    incomplete_props.push_back({rule, val});
  }
}

bool lefTechLayerAreaRuleParser::parseSubRule(
    std::string s,
    odb::dbTechLayer* layer,
    std::vector<std::pair<odb::dbObject*, std::string>>& incomplete_props)
{
  odb::dbTechLayerAreaRule* rule = odb::dbTechLayerAreaRule::create(layer);

  qi::rule<std::string::iterator, space_type> EXCEPT_EDGE_LENGTH
      = ((lit("EXCEPTEDGELENGTH") >> double_ >> double_)[boost::bind(
             &lefTechLayerAreaRuleParser::setExceptEdgeLengths, this, _1, rule)]
         | lit("EXCEPTEDGELENGTH") >> double_[boost::bind(
               &lefTechLayerAreaRuleParser::setInt,
               this,
               _1,
               rule,
               &dbTechLayerAreaRule::setExceptEdgeLength)]);

  qi::rule<std::string::iterator, space_type> EXCEPT_MIN_SIZE
      = (lit("EXCEPTMINSIZE") >> double_ >> double_)[boost::bind(
          &lefTechLayerAreaRuleParser::setExceptMinSize, this, _1, rule)];

  qi::rule<std::string::iterator, space_type> EXCEPT_STEP
      = (lit("EXCEPTSTEP") >> double_ >> double_)[boost::bind(
          &lefTechLayerAreaRuleParser::setExceptStep, this, _1, rule)];

  qi::rule<std::string::iterator, space_type> LAYER
      = ((lit("LAYER")
          >> _string[boost::bind(&lefTechLayerAreaRuleParser::setTrimLayer,
                                 this,
                                 _1,
                                 rule,
                                 layer,
                                 boost::ref(incomplete_props))])
         >> lit("OVERLAP")
         >> int_[boost::bind(&odb::dbTechLayerAreaRule::setOverlap, rule, _1)]);

  qi::rule<std::string::iterator, space_type> AREA
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
