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
  dbTechLayerImplantWidthRule* rule
      = dbTechLayerImplantWidthRule::create(layer);
  std::vector<lefinReader::PendingLayerRef> local_pending;
  bool seen_layer = false;
  bool duplicate_layer = false;
  auto defer = [&](const std::string& name, auto setter) {
    deferLayer(local_pending, rule, layer, "LEF58_WIDTH", name, setter);
  };

  qi::rule<std::string::const_iterator, space_type> keyword
      = lexeme[(lit("LAYER") | lit("ZEROPRL") | lit("MAXWIDTH")
                | lit("EXCEPTCORNERTOUCH") | lit("LENGTH") | lit("CELLHEIGHT")
                | lit("CHECKIMPLANTGROUP") | lit("WITHINLAYER"))
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
            if (name == "ANY") {
              rule->setLayerAny(true);
            } else {
              defer(name, &dbTechLayerImplantWidthRule::setSecondLayer);
            }
          })])
         | lit("ZEROPRL")[boost::bind(
             &dbTechLayerImplantWidthRule::setZeroPrl, rule, true)]
         | (lit("MAXWIDTH") >> double_[dbdistSetter(
                &dbTechLayerImplantWidthRule::setMaxWidth, rule, lefin)])
         | lit("EXCEPTCORNERTOUCH")[boost::bind(
             &dbTechLayerImplantWidthRule::setExceptCornerTouch, rule, true)]
         | (lit("LENGTH") >> double_[dbdistSetter(
                &dbTechLayerImplantWidthRule::setLength, rule, lefin)])
         | (lit("CELLHEIGHT") >> double_[dbdistSetter(
                &dbTechLayerImplantWidthRule::setCellHeight, rule, lefin)])
         | (lit("CHECKIMPLANTGROUP") >> layer_ident[boost::bind(
                &dbTechLayerImplantWidthRule::setCheckImplantGroup, rule, _1)])
         | (lit("WITHINLAYER") >> +layer_ident[([&](const std::string& name) {
              defer(name, &dbTechLayerImplantWidthRule::addWithinLayer);
            })]));

  qi::rule<std::string::const_iterator, space_type> width_rule
      = (lit("WIDTH") >> double_[dbdistSetter(
             &dbTechLayerImplantWidthRule::setMinWidth, rule, lefin)]
         >> *option >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, width_rule, space) && first == last
               && !duplicate_layer;
  if (valid) {
    pending_refs.insert(
        pending_refs.end(), local_pending.begin(), local_pending.end());
  } else {
    dbTechLayerImplantWidthRule::destroy(rule);
  }
  return valid;
}

}  // namespace

bool lefTechLayerImplantWidthParser::parse(
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
