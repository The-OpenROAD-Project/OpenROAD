// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include <string>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

using namespace odb;
void MetalWidthViaMapParser::addEntry(
    boost::fusion::vector<std::string,
                          double,
                          double,
                          boost::optional<double>,
                          boost::optional<double>,
                          std::string>& params)
{
  via_map = dbMetalWidthViaMap::create(tech_);
  auto layer_name = at_c<0>(params);
  if (at_c<3>(params).is_initialized()) {
    via_map->setBelowLayerWidthLow(lefin_->dbdist(at_c<1>(params)));
    via_map->setBelowLayerWidthHigh(lefin_->dbdist(at_c<2>(params)));
    via_map->setAboveLayerWidthLow(lefin_->dbdist(at_c<3>(params).value()));
    via_map->setAboveLayerWidthHigh(lefin_->dbdist(at_c<4>(params).value()));
  } else {
    via_map->setBelowLayerWidthLow(lefin_->dbdist(at_c<1>(params)));
    via_map->setBelowLayerWidthHigh(lefin_->dbdist(at_c<1>(params)));
    via_map->setAboveLayerWidthLow(lefin_->dbdist(at_c<2>(params)));
    via_map->setAboveLayerWidthHigh(lefin_->dbdist(at_c<2>(params)));
  }
  via_map->setViaName(at_c<5>(params));
  via_map->setViaCutClass(cut_class_);
  incomplete_props_.emplace_back(via_map, layer_name);
}

void MetalWidthViaMapParser::setCutClass()
{
  cut_class_ = true;
}

void MetalWidthViaMapParser::setPGVia()
{
  via_map->setPgVia(true);
}

void MetalWidthViaMapParser::parse(const std::string& s)
{
  std::vector<std::string> rules;
  boost::split(rules, s, boost::is_any_of(";"));
  for (auto& rule : rules) {
    boost::algorithm::trim(rule);
    if (rule.empty()) {
      continue;
    }
    rule += " ; ";
    if (!parseSubRule(rule)) {
      lefin_->warning(299,
                      "parse mismatch in property LEF58_METALWIDTHVIAMAP"
                      ":\"{}\"",
                      rule);
    }
  }
}

bool MetalWidthViaMapParser::parseSubRule(std::string s)
{
  cut_class_ = false;
  qi::rule<std::string::iterator, std::string(), ascii::space_type> string_;
  string_ %= lexeme[(alpha >> *(char_ - ' ' - '\n'))];
  qi::rule<std::string::iterator, space_type> ENTRY
      = (lit("VIA") >> (string_ >> double_ >> double_ >> -double_ >> -double_
                        >> string_)[boost::bind(
             &MetalWidthViaMapParser::addEntry, this, _1)]
         >> -lit(
             "PGVIA")[boost::bind(&MetalWidthViaMapParser::setPGVia, this)]);
  qi::rule<std::string::iterator, space_type> METALWIDTHVIAMAP
      = (lit("METALWIDTHVIAMAP") >> -lit("USEVIACUTCLASS")[boost::bind(
             &MetalWidthViaMapParser::setCutClass, this)]
         >> +ENTRY >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid
      = qi::phrase_parse(first, last, METALWIDTHVIAMAP, space) && first == last;
  if (!valid && via_map != nullptr) {
    odb::dbMetalWidthViaMap::destroy(via_map);
    incomplete_props_.pop_back();
  }
  return valid;
}
