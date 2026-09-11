// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <string>
#include <string_view>

#include "boost/bind/bind.hpp"
#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb {

lefTechLayerCutEnclosureTableRuleParser::
    lefTechLayerCutEnclosureTableRuleParser(lefinReader* l)
    : lefin_(l)
{
}

void lefTechLayerCutEnclosureTableRuleParser::checkCutClass(
    const std::string& val,
    odb::dbTechLayer* layer)
{
  auto cutClass = layer->findTechLayerCutClassRule(val.c_str());
  if (cutClass == nullptr) {
    lefin_->warning(
        602,
        "cut class {} not found for LEF58_ENCLOSURETABLE rule for layer {}",
        val,
        layer->getName());
  }
}

void lefTechLayerCutEnclosureTableRuleParser::parse(std::string_view s,
                                                    odb::dbTechLayer* layer)
{
  // boost::spirit's shared _string sub-rule (boostParser.h) is hardcoded to
  // std::string::const_iterator, which is a distinct type from
  // std::string_view::const_iterator on this toolchain, so the grammar below
  // still needs a real std::string to parse against.
  const std::string value(s);

  qi::rule<std::string::const_iterator, space_type> cut_class_rule
      = -(lit("CUTCLASS") >> _string)[boost::bind(
          &lefTechLayerCutEnclosureTableRuleParser::checkCutClass,
          this,
          _1,
          layer)];

  qi::rule<std::string::const_iterator, space_type> above_below_rule
      = -(lit("ABOVE") | lit("BELOW"));

  // Trim-metal-aware overhang row (references a TRIMMETAL layer such as CM1).
  // Trim layers are not modeled in OpenROAD yet, so the clause is recognized
  // and discarded like the rest of the rule.
  qi::rule<std::string::const_iterator, space_type> layer_overlap_rule
      = -(lit("LAYER") >> _string >> -(lit("OVERLAP") >> int_));

  // A single WIDTH (or DEFAULT) value may be followed by more than one
  // overhang quadruple, each an alternative choice of overhang values (the
  // last one is typically flagged MINSUM to indicate the two opposite-side
  // overhangs are tradeable against each other).
  qi::rule<std::string::const_iterator, space_type> overhang_group_rule
      = (above_below_rule >> double_ >> double_ >> double_ >> double_
         >> -lit("MINSUM") >> layer_overlap_rule);

  qi::rule<std::string::const_iterator, space_type> default_row_rule
      = (lit("DEFAULT") >> +overhang_group_rule);

  qi::rule<std::string::const_iterator, space_type> width_row_rule
      = (lit("WIDTH") >> double_ >> +overhang_group_rule);

  qi::rule<std::string::const_iterator, space_type> enclosure_table_rule
      = (lit("ENCLOSURETABLE") >> cut_class_rule >> *default_row_rule
         >> +width_row_rule >> lit(";"));

  auto first = value.begin();
  auto last = value.end();
  bool valid = qi::phrase_parse(first, last, enclosure_table_rule, space)
               && first == last;
  if (!valid) {
    lefin_->warning(603,
                    "parse mismatch in layer property LEF58_ENCLOSURETABLE "
                    "for layer {} :\"{}\"",
                    layer->getName(),
                    s);
  }
}

}  // namespace odb
