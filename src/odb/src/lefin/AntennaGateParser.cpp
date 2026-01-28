// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boost/spirit/home/qi/detail/parse_auto.hpp"
#include "boost/spirit/home/qi/nonterminal/rule.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "lefiLayer.hpp"
#include "odb/lefin.h"

namespace odb {

namespace {
void setDiff(LefParser::lefiLayer* layer, double param)
{
  if (layer->numAntennaModel() == 0) {
    layer->addAntennaModel(1);
  }

  for (int i = 0; i < layer->numAntennaModel(); i++) {
    layer->antennaModel(i)->setAntennaGatePlusDiff(param);
  }
}
}  // namespace

void AntennaGatePlusDiffParser::parse(const std::string& s)
{
  qi::rule<std::string::const_iterator, space_type> LEF57_ANTENNAGATEPLUSDIFF
      = lit("ANTENNAGATEPLUSDIFF") >> double_[boost::bind(&setDiff, layer_, _1)]
        >> lit(";");
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, LEF57_ANTENNAGATEPLUSDIFF, space)
               && first == last;

  if (!valid) {  // fail if we did not get a full match
    lefin_->warning(
        279,
        "parse mismatch in layer property LEF57_ANTENNAGATEPLUSDIFF for "
        "layer {} :\"{}\"",
        layer_->name(),
        s);
  }
}

}  // namespace odb
