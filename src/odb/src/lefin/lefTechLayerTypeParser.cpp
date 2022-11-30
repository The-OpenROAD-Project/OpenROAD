/*
 * Copyright (c) 2019, The Regents of the University of California
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

#include <iostream>
#include <string>

#include "boostParser.h"
#include "db.h"
#include "lefLayerPropParser.h"
#include "lefin.h"

namespace lefTechLayerType {

template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefin* lefin)
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
}  // namespace lefTechLayerType

namespace odb {

bool lefTechLayerTypeParser::parse(std::string s,
                                   dbTechLayer* layer,
                                   odb::lefin* l)
{
  return lefTechLayerType::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
