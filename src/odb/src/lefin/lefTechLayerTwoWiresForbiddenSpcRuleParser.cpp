///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <functional>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerTwoWiresForbiddenSpcRuleParser::
    lefTechLayerTwoWiresForbiddenSpcRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerTwoWiresForbiddenSpcRuleParser::parse(std::string s,
                                                       odb::dbTechLayer* layer)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto& rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty()) {
      continue;
    }
    rule += " ; ";
    if (!parseSubRule(rule, layer)) {
      lefin_->warning(
          438,
          "parse mismatch in layer propery LEF58_FORBIDDENSPACING for "
          "layer {} :\"{}\"",
          layer->getName(),
          rule);
    }
  }
}

void lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt(
    double val,
    odb::dbTechLayerTwoWiresForbiddenSpcRule* rule,
    void (odb::dbTechLayerTwoWiresForbiddenSpcRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}

void lefTechLayerTwoWiresForbiddenSpcRuleParser::setForbiddenSpacing(
    const boost::fusion::vector<double, double>& params,
    odb::dbTechLayerTwoWiresForbiddenSpcRule* rule)
{
  rule->setMinSpacing(lefin_->dbdist(at_c<0>(params)));
  rule->setMaxSpacing(lefin_->dbdist(at_c<1>(params)));
}

bool lefTechLayerTwoWiresForbiddenSpcRuleParser::parseSubRule(
    std::string s,
    odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, std::string(), ascii::space_type> _string;
  _string %= lexeme[+(char_ - ' ')];
  odb::dbTechLayerTwoWiresForbiddenSpcRule* rule
      = odb::dbTechLayerTwoWiresForbiddenSpcRule::create(layer);

  qi::rule<std::string::iterator, space_type> forbiddenSpacing
      = (lit("TWOWIRESFORBIDDENSPACING") >> double_ >> double_)[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setForbiddenSpacing,
            this,
            _1,
            rule)]
        >> lit("MINSPANLENGTH") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMinSpanLength)]
        >> -lit("EXACTSPANLENGTH")[boost::bind(
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMinExactSpanLength,
            rule,
            true)]
        >> lit("MAXSPANLENGTH") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMaxSpanLength)]
        >> -lit("EXACTSPANLENGTH")[boost::bind(
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setMaxExactSpanLength,
            rule,
            true)]
        >> lit("PRL") >> double_[boost::bind(
            &lefTechLayerTwoWiresForbiddenSpcRuleParser::setInt,
            this,
            _1,
            rule,
            &odb::dbTechLayerTwoWiresForbiddenSpcRule::setPrl)]
        >> lit(";");
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, forbiddenSpacing, space) && first == last;
  if (!valid) {
    odb::dbTechLayerTwoWiresForbiddenSpcRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
