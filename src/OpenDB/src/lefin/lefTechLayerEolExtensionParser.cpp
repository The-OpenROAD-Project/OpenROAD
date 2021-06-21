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

lefTechLayerEolExtensionRuleParser::lefTechLayerEolExtensionRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerEolExtensionRuleParser::parse(std::string s,
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
      lefin_->warning(260,
                      "parse mismatch in layer propery "
                      "LEF58_EOLEXTENSIONSPACING for layer {} :\"{}\"",
                      layer->getName(),
                      rule);
  }
}

void lefTechLayerEolExtensionRuleParser::setInt(
    double val,
    odb::dbTechLayerEolExtensionRule* rule,
    void (odb::dbTechLayerEolExtensionRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}
void lefTechLayerEolExtensionRuleParser::addEntry(
    boost::fusion::vector<double, double>& params,
    odb::dbTechLayerEolExtensionRule* rule)
{
  double eol = at_c<0>(params);
  double ext = at_c<1>(params);
  rule->addEntry(lefin_->dbdist(eol), lefin_->dbdist(ext));
}
bool lefTechLayerEolExtensionRuleParser::parseSubRule(std::string s,
                                                      odb::dbTechLayer* layer)
{
  odb::dbTechLayerEolExtensionRule* rule
      = odb::dbTechLayerEolExtensionRule::create(layer);

  qi::rule<std::string::iterator, space_type> EXTENSION_ENTRY
      = (lit("ENDOFLINE") >> double_ >> lit("EXTENSION")
         >> double_)[boost::bind(
          &lefTechLayerEolExtensionRuleParser::addEntry, this, _1, rule)];

  qi::rule<std::string::iterator, space_type> EOLEXTENSIONSPACING
      = (lit("EOLEXTENSIONSPACING")
         >> double_[boost::bind(&lefTechLayerEolExtensionRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerEolExtensionRule::setSpacing)]
         >> -lit("PARALLELONLY")[boost::bind(
             &odb::dbTechLayerEolExtensionRule::setParallelOnly, rule, true)]
         >> +EXTENSION_ENTRY >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, EOLEXTENSIONSPACING, space);
  return valid && first == last;
}

}  // namespace odb
