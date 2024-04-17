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

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace lefTechLayerCutClass {

void addCutClassRule(
    boost::fusion::vector<std::string,
                          double,
                          boost::optional<double>,
                          boost::optional<int>,
                          boost::optional<std::string>>& params,
    odb::dbTechLayer* layer,
    odb::lefin* lefin)
{
  std::string name = at_c<0>(params);
  auto rule = odb::dbTechLayerCutClassRule::create(layer, name.c_str());
  rule->setWidth(lefin->dbdist(at_c<1>(params)));
  auto length = at_c<2>(params);
  auto cnt = at_c<3>(params);
  auto orient = at_c<4>(params);
  if (length.is_initialized()) {
    rule->setLengthValid(true);
    rule->setLength(lefin->dbdist(length.value()));
  }
  if (cnt.is_initialized()) {
    rule->setCutsValid(true);
    rule->setNumCuts(cnt.value());
  }
  if (orient.is_initialized()) {
    lefin->warning(
        421, "Keyword ORIENT is not supported in CUTCLASS {}.", name);
  }
}

}  // namespace lefTechLayerCutClass

namespace odb {
bool lefTechLayerCutClassParser::parse(std::string s,
                                       dbTechLayer* layer,
                                       odb::lefin* lefin)
{
  auto first = s.begin();
  auto last = s.end();
  qi::rule<std::string::iterator, space_type> cutClassRule
      = (+(lit("CUTCLASS") >> _string >> lit("WIDTH") >> double_
           >> -(lit("LENGTH") >> double_) >> -(lit("CUTS") >> int_)
           >> -(lit("ORIENT") >> _string) >> lit(";"))[boost::bind(
          &lefTechLayerCutClass::addCutClassRule, _1, layer, lefin)]);

  bool valid
      = qi::phrase_parse(first, last, cutClassRule, space) && first == last;
  return valid;
}

}  // namespace odb
