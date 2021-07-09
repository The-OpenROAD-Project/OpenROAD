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

#include <functional>
#include <string>

#include "boostParser.h"
#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace odb {

lefTechLayerEolKeepOutRuleParser::lefTechLayerEolKeepOutRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerEolKeepOutRuleParser::parse(std::string s,
                                             odb::dbTechLayer* layer)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty())
      continue;
    rule += " ; ";
    if (!parseSubRule(rule, layer))
      lefin_->warning(280,
                      "parse mismatch in layer propery LEF58_EOLKEEPOUT for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
  }
}
void lefTechLayerEolKeepOutRuleParser::setClass(
    std::string val,
    odb::dbTechLayerEolKeepOutRule* rule,
    odb::dbTechLayer* layer)
{
  rule->setClassName(val);
  rule->setClassValid(true);
}

void lefTechLayerEolKeepOutRuleParser::setExceptWithin(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerEolKeepOutRule* rule)
{
  rule->setExceptWithin(true);
  rule->setWithinLow(lefin_->dbdist(at_c<0>(params)));
  rule->setWithinHigh(lefin_->dbdist(at_c<1>(params)));
}

void lefTechLayerEolKeepOutRuleParser::setInt(
    double val,
    odb::dbTechLayerEolKeepOutRule* rule,
    void (odb::dbTechLayerEolKeepOutRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}
bool lefTechLayerEolKeepOutRuleParser::parseSubRule(std::string s,
                                                    odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  odb::dbTechLayerEolKeepOutRule* rule
      = odb::dbTechLayerEolKeepOutRule::create(layer);
  qi::rule<std::string::iterator, space_type> EXCEPTWITHIN
      = (lit("EXCEPTWITHIN") >> double_ >> double_)[boost::bind(
          &lefTechLayerEolKeepOutRuleParser::setExceptWithin, this, _1, rule)];
  qi::rule<std::string::iterator, space_type> CLASS
      = (lit("CLASS")
         >> _string[boost::bind(&lefTechLayerEolKeepOutRuleParser::setClass,
                                this,
                                _1,
                                rule,
                                layer)]);
  qi::rule<std::string::iterator, space_type> EOLKEEPOUT
      = (lit("EOLKEEPOUT")
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setEolWidth)]
         >> lit("EXTENSION") >> double_[boost::bind(
             &lefTechLayerEolKeepOutRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerEolKeepOutRule::setBackwardExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setSideExt)]
         >> double_[boost::bind(&lefTechLayerEolKeepOutRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolKeepOutRule::setForwardExt)]
         >> -EXCEPTWITHIN >> -CLASS >> -lit("CORNERONLY")[boost::bind(
             &odb::dbTechLayerEolKeepOutRule::setCornerOnly, rule, true)]
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, EOLKEEPOUT, space);
  return valid && first == last;
}

}  // namespace odb
