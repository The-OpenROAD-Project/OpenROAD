// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

namespace {

void setMinEncRule(
    const boost::fusion::vector<double, boost::optional<double>>& params,
    dbTechMinEncRule* rule,
    lefinReader* lefin)
{
  rule->setEnclosure(lefin->dbarea(at_c<0>(params)));
  if (at_c<1>(params).is_initialized()) {
    rule->setEnclosureWidth(lefin->dbdist(at_c<1>(params).value()));
  }
}

bool parseSubRule(const std::string& s, dbTechLayer* layer, lefinReader* lefin)
{
  dbTechMinEncRule* rule = dbTechMinEncRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> min_enc_rule
      = (lit("MINENCLOSEDAREA") >> double_ >> -(lit("WIDTH") >> double_)
         >> lit(";"))[boost::bind(&setMinEncRule, _1, rule, lefin)];

  auto first = s.begin();
  auto last = s.end();
  const bool valid
      = qi::phrase_parse(first, last, min_enc_rule, space) && first == last;
  if (!valid) {
    dbTechMinEncRule::destroy(rule);
  }
  return valid;
}

}  // namespace

bool lefTechLayerMinEnclosedAreaParser::parse(const std::string& s,
                                              dbTechLayer* layer,
                                              lefinReader* lefin)
{
  return parseAllRules(s, [&](const std::string& rule) {
    return parseSubRule(rule, layer, lefin);
  });
}

}  // namespace odb
