/*
 * Copyright (c) 2023, The Regents of the University of California
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
#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerPitchRuleParser::lefTechLayerPitchRuleParser(lefin* l)
{
  lefin_ = l;
}

void lefTechLayerPitchRuleParser::parse(std::string s, odb::dbTechLayer* layer)
{
  qi::rule<std::string::iterator, space_type> PITCH
      = ((lit("PITCH") >> double_ >> double_)[boost::bind(
             &odb::lefTechLayerPitchRuleParser::setPitchXY, this, _1, layer)]
         | lit("PITCH")
               >> double_[boost::bind(&odb::lefTechLayerPitchRuleParser::setInt,
                                      this,
                                      _1,
                                      layer,
                                      &odb::dbTechLayer::setPitch)]);

  qi::rule<std::string::iterator, space_type> FIRST_LAST_PTICH
      = (PITCH
         >> -(lit("FIRSTLASTPITCH")
              >> double_[boost::bind(&odb::lefTechLayerPitchRuleParser::setInt,
                                     this,
                                     _1,
                                     layer,
                                     &odb::dbTechLayer::setFirstLastPitch)])
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, FIRST_LAST_PTICH, space) && first == last;
  if (!valid) {
    lefin_->warning(281,
                    "parse mismatch in layer propery LEF58_Pitch for layer");
    layer->setPitch(0);
    layer->setFirstLastPitch(-1);
  }
}

void lefTechLayerPitchRuleParser::setInt(double val,
                                         odb::dbTechLayer* layer,
                                         void (odb::dbTechLayer::*func)(int))
{
  (layer->*func)(lefin_->dbdist(val));
}

void lefTechLayerPitchRuleParser::setPitchXY(
    boost::fusion::vector<double, double>& params,
    odb::dbTechLayer* layer)
{
  layer->setPitchXY(lefin_->dbdist(at_c<0>(params)),
                    lefin_->dbdist(at_c<1>(params)));
}

}  // namespace odb
