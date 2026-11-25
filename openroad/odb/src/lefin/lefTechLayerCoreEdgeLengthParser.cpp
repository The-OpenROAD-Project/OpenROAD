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

void addCoreEdgeExceptAdj(
    const boost::fusion::vector<double, double, bool>& params,
    dbTechLayerCoreEdgeLengthRule* rule,
    lefinReader* lefin)
{
  rule->addExceptAdjacentLength(lefin->dbdist(at_c<0>(params)),
                                lefin->dbdist(at_c<1>(params)),
                                at_c<2>(params));
}

bool parseSubRule(const std::string& s,
                  dbTechLayer* layer,
                  lefinReader* lefin,
                  std::vector<lefinReader::PendingLayerRef>& pending_refs)
{
  dbTechLayerCoreEdgeLengthRule* rule
      = dbTechLayerCoreEdgeLengthRule::create(layer);
  std::vector<lefinReader::PendingLayerRef> local_pending;
  bool seen_max_len = false;
  bool duplicate_max_len = false;
  auto defer = [&](const std::string& name, auto setter) {
    deferLayer(
        local_pending, rule, layer, "LEF58_COREEDGELENGTH", name, setter);
  };

  qi::rule<std::string::const_iterator, space_type> keyword
      = lexeme[(lit("MAXLENGTH") | lit("EXCEPTADJACENTLENGTH")
                | lit("EXACTADJACENTLENGTH") | lit("WITHINLAYER")
                | lit("EXCEPTLAYER"))
               >> !(char_ - blank - '\n' - ';')];

  qi::rule<std::string::const_iterator, std::string(), space_type> layer_ident
      = (_string - keyword - lit(";"));

  // LEF 5.8 syntax (Fig. 3-2): {exactEdgeLength adjLength
  // [EXACTADJACENTLENGTH]}... EXACTADJACENTLENGTH is a suffix flag on the
  // preceding (edge, adj) pair.
  qi::rule<std::string::const_iterator, space_type> except_adj_entry
      = (double_ >> double_
         >> qi::matches[lit("EXACTADJACENTLENGTH")])[boost::bind(
          &addCoreEdgeExceptAdj, _1, rule, lefin)];

  qi::rule<std::string::const_iterator, space_type> option
      = ((lit("MAXLENGTH") >> double_[([&](double val) {
            if (seen_max_len) {
              duplicate_max_len = true;
              return;
            }
            seen_max_len = true;
            rule->setMaxLength(lefin->dbdist(val));
          })])
         | (lit("EXCEPTADJACENTLENGTH") >> +except_adj_entry)
         | (lit("WITHINLAYER") >> +layer_ident[([&](const std::string& name) {
              defer(name, &dbTechLayerCoreEdgeLengthRule::addWithinLayer);
            })])
         | (lit("EXCEPTLAYER") >> +layer_ident[([&](const std::string& name) {
              defer(name, &dbTechLayerCoreEdgeLengthRule::addExceptLayer);
            })]));

  qi::rule<std::string::const_iterator, space_type> core_edge_rule
      = (lit("COREEDGELENGTH") >> double_[dbdistSetter(
             &dbTechLayerCoreEdgeLengthRule::setMinLength, rule, lefin)]
         >> *option >> lit(";"));

  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, core_edge_rule, space)
               && first == last && !duplicate_max_len;
  if (valid) {
    pending_refs.insert(
        pending_refs.end(), local_pending.begin(), local_pending.end());
  } else {
    dbTechLayerCoreEdgeLengthRule::destroy(rule);
  }
  return valid;
}

}  // namespace

bool lefTechLayerCoreEdgeLengthParser::parse(
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
