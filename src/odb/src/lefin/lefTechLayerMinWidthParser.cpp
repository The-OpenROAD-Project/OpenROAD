// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boost/fusion/container/vector/vector.hpp"
#include "boost/optional/optional.hpp"
#include "boost/spirit/home/qi/detail/parse_auto.hpp"
#include "boost/spirit/home/qi/nonterminal/rule.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

namespace {

void setMinWidth(
    const boost::fusion::vector<double, boost::optional<std::string>>& params,
    odb::dbTechLayer* layer,
    odb::lefinReader* lefinReader)
{
  if (at_c<1>(params).is_initialized()) {
    layer->setWrongWayMinWidth(lefinReader->dbdist(at_c<0>(params)));
  } else {
    layer->setMinWidth(lefinReader->dbdist(at_c<0>(params)));
  }
}

template <typename Iterator>
bool parseRule(Iterator first,
               Iterator last,
               odb::dbTechLayer* layer,
               odb::lefinReader* lefinReader)
{
  qi::rule<std::string::const_iterator, space_type> MinWidthRule
      = lit("MINWIDTH") >> (double_ >> -string("WRONGDIRECTION"))[boost::bind(
            &setMinWidth, _1, layer, lefinReader)]
        >> lit(";");

  bool valid = qi::phrase_parse(first, last, MinWidthRule, space);

  return valid && first == last;
}
}  // namespace

void MinWidthParser::parse(const std::string& s)
{
  if (!parseRule(s.begin(), s.end(), layer_, lefin_)) {
    lefin_->warning(
        282,
        "parse mismatch in layer property LEF58_MINWIDTH for layer {} :\"{}\"",
        layer_->getName(),
        s);
  }
}

}  // namespace odb
