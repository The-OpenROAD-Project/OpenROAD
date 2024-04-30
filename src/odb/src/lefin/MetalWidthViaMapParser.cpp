/* Authors: Osama */
/*
 * Copyright (c) 2022, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <functional>
#include <iostream>
#include <string>

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
  incomplete_props_.push_back({via_map, layer_name});
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
                      "parse mismatch in propery LEF58_METALWIDTHVIAMAP"
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
