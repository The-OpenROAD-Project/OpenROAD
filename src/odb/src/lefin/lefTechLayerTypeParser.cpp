// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerType {

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader)
{
  qi::rule<std::string::iterator, space_type> TypeRule
      = (lit("TYPE")
         >> (lit("NWELL")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                      layer,
                                      odb::dbTechLayer::LEF58_TYPE::NWELL)]
             | lit("PWELL")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                        layer,
                                        odb::dbTechLayer::LEF58_TYPE::PWELL)]
             | lit("ABOVEDIEEDGE")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::ABOVEDIEEDGE)]
             | lit("BELOWDIEEDGE")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::BELOWDIEEDGE)]
             | lit("DIFFUSION")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::DIFFUSION)]
             | lit("TRIMPOLY")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::TRIMPOLY)]
             | lit("MIMCAP")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                         layer,
                                         odb::dbTechLayer::LEF58_TYPE::MIMCAP)]
             | lit("STACKEDMIMCAP")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::STACKEDMIMCAP)]
             | lit("TSVMETAL")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::TSVMETAL)]
             | lit("PASSIVATION")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::PASSIVATION)]
             | lit("HIGHR")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                        layer,
                                        odb::dbTechLayer::LEF58_TYPE::HIGHR)]
             | lit("TRIMMETAL")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::TRIMMETAL)]
             | lit("REGION")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                         layer,
                                         odb::dbTechLayer::LEF58_TYPE::REGION)]
             | lit("MEOL")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                       layer,
                                       odb::dbTechLayer::LEF58_TYPE::MEOL)]
             | lit("WELLDISTANCE")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::WELLDISTANCE)]
             | lit("CPODE")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                        layer,
                                        odb::dbTechLayer::LEF58_TYPE::CPODE)]
             | lit("TSV")[boost::bind(&odb::dbTechLayer::setLef58Type,
                                      layer,
                                      odb::dbTechLayer::LEF58_TYPE::TSV)]
             | lit("PADMETAL")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::PADMETAL)]
             | lit("POLYROUTING")[boost::bind(
                 &odb::dbTechLayer::setLef58Type,
                 layer,
                 odb::dbTechLayer::LEF58_TYPE::POLYROUTING)])
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, TypeRule, space);

  return valid && first == last;
}
}  // namespace odb::lefTechLayerType

namespace odb {

bool lefTechLayerTypeParser::parse(std::string s,
                                   dbTechLayer* layer,
                                   odb::lefinReader* l)
{
  return lefTechLayerType::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
