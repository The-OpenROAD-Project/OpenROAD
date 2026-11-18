// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include <string>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boost/spirit/home/qi/detail/parse_auto.hpp"
#include "boost/spirit/home/qi/nonterminal/rule.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "lefiLayer.hpp"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

namespace {
// No global helpers needed if we use local struct
}  // namespace

void AntennaGatePlusDiffParser::parse(const std::string& s)
{
  struct ParserState
  {
    dbTechLayer* layer;
    lefinReader* lefin;
    std::string oxide = "OXIDE1";
    std::vector<double> idx;
    std::vector<double> ratios;

    double temp_x = 0.0;

    void setOxide(const std::string& name) { oxide = name; }

    dbTechLayerAntennaRule* getRule()
    {
      if (oxide == "OXIDE2") {
        return layer->getOrCreateAntennaModel(/*oxide_idx=*/2);
      }

      if (oxide == "OXIDE1" || oxide.empty()) {
        return layer->getOrCreateAntennaModel(/*oxide_idx=*/1);
      }

      lefin->warning(
          1000, "Unsupported oxide (Maximum of 2 supported): {}", oxide);
      return nullptr;
    }

    void setFactor(double factor)
    {
      auto rule = getRule();
      if (rule) {
        rule->setGatePlusDiffFactor(factor);
      }
    }

    void setX(double x) { temp_x = x; }

    void setY(double y)
    {
      idx.push_back(temp_x);
      ratios.push_back(y);
    }

    void setPWL()
    {
      auto rule = getRule();
      if (rule) {
        rule->setGatePlusDiffPWL(idx, ratios);
      }
    }
  } state;

  state.layer = layer_;
  state.lefin = lefin_;

  namespace qi = boost::spirit::qi;

  qi::rule<std::string::const_iterator, space_type> pair_rule
      = (lit("(") >> double_[boost::bind(&ParserState::setX, &state, _1)]
         >> double_[boost::bind(&ParserState::setY, &state, _1)] >> lit(")"));

  qi::rule<std::string::const_iterator, space_type> pwl_rule
      = lit("PWL") >> lit("(") >> +pair_rule >> lit(")");

  qi::rule<std::string::const_iterator, space_type> oxide_rule
      = lit("OXIDE1")[boost::bind(&ParserState::setOxide, &state, "OXIDE1")]
        | lit("OXIDE2")[boost::bind(&ParserState::setOxide, &state, "OXIDE2")]
        | lit("OXIDE3")[boost::bind(&ParserState::setOxide, &state, "OXIDE3")];

  qi::rule<std::string::const_iterator, space_type> grammar
      = lit("ANTENNAGATEPLUSDIFF") >> -oxide_rule
        >> (double_[boost::bind(&ParserState::setFactor, &state, _1)]
            | pwl_rule[boost::bind(&ParserState::setPWL, &state)])
        >> lit(";");

  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, grammar, space) && first == last;

  if (!valid) {
    lefin_->warning(279,
                    "parse mismatch in layer property ANTENNAGATEPLUSDIFF for "
                    "layer {} :\"{}\"",
                    layer_->getName(),
                    s);
  }
}

}  // namespace odb
