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

#include "boostParser.h"
#include <iostream>
#include <string>

#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace lefTechLayerRightWayOnGridOnly {

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefin* lefin)
{
  qi::rule<std::string::iterator, space_type> rightWayOnGridOnlyRule
      = (lit("RIGHTWAYONGRIDONLY")[boost::bind(
             &odb::dbTechLayer::setRightWayOnGridOnly, layer, true)]
         >> -lit("CHECKMASK")[boost::bind(
             &odb::dbTechLayer::setRightWayOnGridOnlyCheckMask, layer, true)]
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, rightWayOnGridOnlyRule, space);
  return valid && first == last;
}
}  // namespace lefTechLayerRightWayOnGridOnly

namespace odb {

bool lefTechLayerRightWayOnGridOnlyParser::parse(std::string s,
                                                 dbTechLayer* layer,
                                                 odb::lefin* l)
{
  return lefTechLayerRightWayOnGridOnly::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
