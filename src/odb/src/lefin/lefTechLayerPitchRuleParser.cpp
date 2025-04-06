// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include <functional>
#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerPitchRuleParser::lefTechLayerPitchRuleParser(lefinReader* l)
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
                    "parse mismatch in layer property LEF58_Pitch for layer");
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
