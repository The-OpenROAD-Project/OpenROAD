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

void lefTechLayerCutEnclosureTableRuleParser::setCutClass(
    const std::string& val,
    odb::dbTechLayer* layer)
{
  auto cutClass = layer->findTechLayerCutClassRule(val.c_str());
  if (cutClass == nullptr) {
    ruleValid_ = false;
    lefin_->warning(
        602,
        "cut class {} not found for LEF58_ENCLOSURETABLE rule for layer {}",
        val,
        layer->getName());
    return;
  }
  curRule_->setCutClass(cutClass);
  curRule_->setCutClassValid(true);
}

void lefTechLayerCutEnclosureTableRuleParser::setWidth(double val)
{
  curWidth_ = lefin_->dbdist(val);
}

void lefTechLayerCutEnclosureTableRuleParser::addDefaultRow(
    boost::fusion::vector<int, double, double, double, double>& params)
{
  curRule_->addDefaultRow({at_c<0>(params),
                           lefin_->dbdist(at_c<1>(params)),
                           lefin_->dbdist(at_c<2>(params)),
                           lefin_->dbdist(at_c<3>(params)),
                           lefin_->dbdist(at_c<4>(params))});
}

void lefTechLayerCutEnclosureTableRuleParser::addWidthRow(
    boost::fusion::vector<int, double, double, double, double>& params)
{
  curRule_->addWidthRow({curWidth_,
                         at_c<0>(params),
                         lefin_->dbdist(at_c<1>(params)),
                         lefin_->dbdist(at_c<2>(params)),
                         lefin_->dbdist(at_c<3>(params)),
                         lefin_->dbdist(at_c<4>(params))});
}

void lefTechLayerCutEnclosureTableRuleParser::parse(std::string_view s,
                                                    odb::dbTechLayer* layer)
{
  // boost::spirit's shared _string sub-rule (boostParser.h) is hardcoded to
  // std::string::const_iterator, which is a distinct type from
  // std::string_view::const_iterator on this toolchain, so the grammar below
  // still needs a real std::string to parse against.
  const std::string value(s);

  ruleValid_ = true;
  curRule_ = odb::dbTechLayerCutEnclosureTableDefRule::create(layer);
  curWidth_ = 0;

  qi::rule<std::string::const_iterator, space_type> cut_class_rule
      = (lit("CUTCLASS") >> _string)[boost::bind(
          &lefTechLayerCutEnclosureTableRuleParser::setCutClass,
          this,
          _1,
          layer)];

  // above_below: 0 = both, 1 = above, 2 = below
  qi::rule<std::string::const_iterator, int(), space_type> above_below_rule
      = (lit("ABOVE") >> qi::attr(1)) | (lit("BELOW") >> qi::attr(2))
        | qi::attr(0);

  // Trim-metal-aware overhang row (references a TRIMMETAL layer such as CM1).
  // Trim layers are not modeled in OpenROAD yet, so the clause is recognized
  // and discarded like the rest of the rule.
  qi::rule<std::string::const_iterator, space_type> layer_overlap_rule
      = (lit("LAYER") >> _string >> -(lit("OVERLAP") >> int_));

  // A single WIDTH (or DEFAULT) value may be followed by more than one
  // overhang quadruple, each an alternative choice of overhang values.
  // MINSUM is intentionally not supported.
  qi::rule<std::string::const_iterator,
           boost::fusion::vector<int, double, double, double, double>(),
           space_type>
      overhang_group_rule = (above_below_rule >> double_ >> double_ >> double_
                             >> double_ >> -layer_overlap_rule);

  qi::rule<std::string::const_iterator, space_type> default_row_rule
      = lit("DEFAULT") >> +(overhang_group_rule[boost::bind(
            &lefTechLayerCutEnclosureTableRuleParser::addDefaultRow,
            this,
            _1)]);

  qi::rule<std::string::const_iterator, space_type> width_row_rule
      = lit("WIDTH") >> double_[boost::bind(
            &lefTechLayerCutEnclosureTableRuleParser::setWidth, this, _1)]
        >> +(overhang_group_rule[boost::bind(
            &lefTechLayerCutEnclosureTableRuleParser::addWidthRow, this, _1)]);

  qi::rule<std::string::const_iterator, space_type> enclosure_table_rule
      = (lit("ENCLOSURETABLE") >> -cut_class_rule >> *default_row_rule
         >> +width_row_rule >> lit(";"));

  auto first = value.begin();
  auto last = value.end();
  bool valid = qi::phrase_parse(first, last, enclosure_table_rule, space)
               && first == last && ruleValid_;
  if (!valid) {
    lefin_->warning(603,
                    "parse mismatch in layer property LEF58_ENCLOSURETABLE "
                    "for layer {} :\"{}\"",
                    layer->getName(),
                    s);
    odb::dbTechLayerCutEnclosureTableDefRule::destroy(curRule_);
  }
  curRule_ = nullptr;
}

}  // namespace odb
