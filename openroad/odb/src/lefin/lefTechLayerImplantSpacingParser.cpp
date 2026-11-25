// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>
#include <vector>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

namespace {

bool parseSubRule(const std::string& s,
                  dbTechLayer* layer,
                  lefinReader* lefin,
                  std::vector<lefinReader::PendingLayerRef>& pending_refs)
{
  dbTechLayerImplantSpacingRule* rule
      = dbTechLayerImplantSpacingRule::create(layer);
  std::vector<lefinReader::PendingLayerRef> local_pending;
  bool seen_layer = false;
  bool duplicate_layer = false;
  bool seen_dir = false;
  bool conflicting_dir = false;
  auto defer = [&](const std::string& name, auto setter) {
    deferLayer(local_pending, rule, layer, "LEF58_SPACING", name, setter);
  };

  qi::rule<std::string::const_iterator, space_type> keyword
      = lexeme[(lit("LAYER") | lit("HORIZONTAL") | lit("VERTICAL") | lit("PRL")
                | lit("EXCEPTABUTTED") | lit("EXCEPTCORNERTOUCH")
                | lit("LENGTH") | lit("INTERSECTLAYERS"))
               >> !(char_ - blank - '\n' - ';')];

  qi::rule<std::string::const_iterator, std::string(), space_type> layer_ident
      = (_string - keyword - lit(";"));

  qi::rule<std::string::const_iterator, space_type> option
      = ((lit("LAYER") >> layer_ident[([&](const std::string& name) {
            if (seen_layer) {
              duplicate_layer = true;
              return;
            }
            seen_layer = true;
            defer(name, &dbTechLayerImplantSpacingRule::setSecondLayer);
          })])
         | lit("HORIZONTAL")[([&] {
             if (seen_dir) {
               conflicting_dir = true;
             }
             seen_dir = true;
             rule->setHorizontal(true);
           })]
         | lit("VERTICAL")[([&] {
             if (seen_dir) {
               conflicting_dir = true;
             }
             seen_dir = true;
             rule->setVertical(true);
           })]
         | (lit("PRL") >> double_[([&](double val) {
              rule->setPrlValid(true);
              rule->setPrl(lefin->dbdist(val));
            })])
         | lit("EXCEPTABUTTED")[boost::bind(
             &dbTechLayerImplantSpacingRule::setExceptAbutted, rule, true)]
         | lit("EXCEPTCORNERTOUCH")[boost::bind(
             &dbTechLayerImplantSpacingRule::setExceptCornerTouch, rule, true)]
         | (lit("LENGTH") >> double_[dbdistSetter(
                &dbTechLayerImplantSpacingRule::setLength, rule, lefin)])
         | (lit("INTERSECTLAYERS")
            >> +layer_ident[([&](const std::string& name) {
                defer(name, &dbTechLayerImplantSpacingRule::addIntersectLayer);
              })]));

  qi::rule<std::string::const_iterator, space_type> spc_rule
      = (lit("SPACING") >> double_[dbdistSetter(
             &dbTechLayerImplantSpacingRule::setMinSpacing, rule, lefin)]
         >> *option >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, spc_rule, space) && first == last
               && !duplicate_layer && !conflicting_dir;
  if (valid) {
    pending_refs.insert(
        pending_refs.end(), local_pending.begin(), local_pending.end());
  } else {
    dbTechLayerImplantSpacingRule::destroy(rule);
  }
  return valid;
}

}  // namespace

bool lefTechLayerImplantSpacingParser::parse(
    const std::string& s,
    dbTechLayer* layer,
    lefinReader* lefin,
    std::vector<lefinReader::PendingLayerRef>& pending_refs)
{
  return parseAllRules(s, [&](const std::string& rule) {
    return parseSubRule(rule, layer, lefin, pending_refs);
  });
}

}  // namespace odb
