/*
 * Copyright (c) 2022 Google LLC
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
#include "lefMacroPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace lefMacroClassType {

/**
 * Convient function to encapsulate endcap types.
 *
 * @param make_lit
 * @return qi::rule<std::string_view::iterator, space_type>
 */
template <class T>
auto endcap(const T& make_lit)
{
  return qi::copy(
      lit("ENDCAP")
      >> (make_lit("BOTTOMEDGE", odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE)
          | make_lit("TOPEDGE", odb::dbMasterType::ENDCAP_LEF58_TOPEDGE)
          | make_lit("RIGHTEDGE", odb::dbMasterType::ENDCAP_LEF58_RIGHTEDGE)
          | make_lit("LEFTEDGE", odb::dbMasterType::ENDCAP_LEF58_LEFTEDGE)
          | make_lit("RIGHTBOTTOMEDGE",
                     odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE)
          | make_lit("LEFTBOTTOMEDGE",
                     odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE)
          | make_lit("RIGHTTOPEDGE",
                     odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE)
          | make_lit("LEFTTOPEDGE", odb::dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE)
          | make_lit("RIGHTBOTTOMCORNER",
                     odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER)
          | make_lit("LEFTBOTTOMCORNER",
                     odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER)
          | make_lit("RIGHTTOPCORNER",
                     odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER)
          | make_lit("LEFTTOPCORNER",
                     odb::dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER)
          | make_lit("PRE", odb::dbMasterType::ENDCAP_PRE)
          | make_lit("POST", odb::dbMasterType::ENDCAP_POST)
          | make_lit("TOPLEFT", odb::dbMasterType::ENDCAP_TOPLEFT)
          | make_lit("TOPRIGHT", odb::dbMasterType::ENDCAP_TOPRIGHT)
          | make_lit("BOTTOMLEFT", odb::dbMasterType::ENDCAP_BOTTOMLEFT)
          | make_lit("BOTTOMRIGHT", odb::dbMasterType::ENDCAP_BOTTOMRIGHT)));
}

template <class T>
auto cover(const T& make_lit)
{
  return qi::copy(make_lit("COVER", odb::dbMasterType::COVER)
                  >> -(make_lit("BUMP", odb::dbMasterType::COVER_BUMP)));
}

template <class T>
auto ring(const T& make_lit)
{
  return qi::copy(make_lit("RING", odb::dbMasterType::RING));
}

template <class T>
auto block(const T& make_lit)
{
  return qi::copy(make_lit("BLOCK", odb::dbMasterType::BLOCK)
                  >> -(make_lit("BLACKBOX", odb::dbMasterType::BLOCK_BLACKBOX))
                  >> -(make_lit("SOFT", odb::dbMasterType::BLOCK_SOFT)));
}

template <class T>
auto core(const T& make_lit)
{
  return qi::copy(
      make_lit("CORE", odb::dbMasterType::CORE)
      >> -(make_lit("FEEDTHRU", odb::dbMasterType::CORE_FEEDTHRU))
      >> -(make_lit("TIEHIGH", odb::dbMasterType::CORE_TIEHIGH))
      >> -(make_lit("TIELOW", odb::dbMasterType::CORE_TIELOW))
      >> -(make_lit("SPACER", odb::dbMasterType::CORE_SPACER))
      >> -(make_lit("ANTENNACELL", odb::dbMasterType::CORE_ANTENNACELL))
      >> -(make_lit("WELLTAP", odb::dbMasterType::CORE_WELLTAP)));
}

template <class T>
auto pad(const T& make_lit)
{
  return qi::copy(make_lit("PAD", odb::dbMasterType::PAD)
                  >> -(make_lit("INPUT", odb::dbMasterType::PAD_INPUT))
                  >> -(make_lit("OUTPUT", odb::dbMasterType::PAD_OUTPUT))
                  >> -(make_lit("INOUT", odb::dbMasterType::PAD_INOUT))
                  >> -(make_lit("POWER", odb::dbMasterType::PAD_POWER))
                  >> -(make_lit("SPACER", odb::dbMasterType::PAD_SPACER))
                  >> -(make_lit("AREAIO", odb::dbMasterType::PAD_AREAIO)));
}

template <typename Iterator>
bool parse(Iterator first, Iterator last, odb::dbMaster* master)
{
  // Create a quick lambda to automatically apply the type to the given master
  // instance.
  auto make_lit = [master](const char* name, odb::dbMasterType::Value type) {
    return qi::copy(
        lit(name)[boost::bind(&odb::dbMaster::setType, master, type)]);
  };

  qi::rule<std::string_view::iterator, space_type> type_rule
      = (lit("CLASS") >> (endcap(make_lit) | cover(make_lit) | ring(make_lit)
                          | block(make_lit) | core(make_lit) | pad(make_lit))
         >> lit(";"));

  bool valid = qi::phrase_parse(first, last, type_rule, space);

  return valid && first == last;
}
}  // namespace lefMacroClassType

namespace odb {

bool lefMacroClassTypeParser::parse(std::string_view s, dbMaster* master)
{
  return lefMacroClassType::parse(s.begin(), s.end(), master);
}

}  // namespace odb
