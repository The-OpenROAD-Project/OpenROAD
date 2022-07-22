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
#include "db.h"
#include "lefMacroPropParser.h"
#include "lefin.h"

namespace lefMacroClassType {

template <typename Iterator>
bool parse(Iterator first, Iterator last, odb::dbMaster* master) {
  qi::rule<std::string_view::iterator, space_type> cover =
      (lit("COVER")[boost::bind(&odb::dbMaster::setType, master,
                                odb::dbMasterType::COVER)] >>
       -(lit("BUMP")[boost::bind(&odb::dbMaster::setType, master,
                                 odb::dbMasterType::COVER_BUMP)]));

  qi::rule<std::string_view::iterator, space_type> ring =
      (lit("RING")[boost::bind(&odb::dbMaster::setType, master,
                               odb::dbMasterType::RING)]);

  qi::rule<std::string_view::iterator, space_type> block =
      (lit("BLOCK")[boost::bind(&odb::dbMaster::setType, master,
                                odb::dbMasterType::BLOCK)] >>
       -(lit("BLACKBOX")[boost::bind(&odb::dbMaster::setType, master,
                                     odb::dbMasterType::BLOCK_BLACKBOX)]) >>
       -(lit("SOFT")[boost::bind(&odb::dbMaster::setType, master,
                                 odb::dbMasterType::BLOCK_SOFT)]));

  qi::rule<std::string_view::iterator, space_type> core =
      (lit("CORE")[boost::bind(&odb::dbMaster::setType, master,
                               odb::dbMasterType::CORE)] >>
       -(lit("FEEDTHRU")[boost::bind(&odb::dbMaster::setType, master,
                                     odb::dbMasterType::CORE_FEEDTHRU)]) >>
       -(lit("TIEHIGH")[boost::bind(&odb::dbMaster::setType, master,
                                    odb::dbMasterType::CORE_TIEHIGH)]) >>
       -(lit("TIELOW")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::CORE_TIELOW)]) >>
       -(lit("SPACER")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::CORE_SPACER)]) >>
       -(lit(
           "ANTENNACELL")[boost::bind(&odb::dbMaster::setType, master,
                                      odb::dbMasterType::CORE_ANTENNACELL)]) >>
       -(lit("WELLTAP")[boost::bind(&odb::dbMaster::setType, master,
                                    odb::dbMasterType::CORE_WELLTAP)]));

  qi::rule<std::string_view::iterator, space_type> pad =
      (lit("PAD")[boost::bind(&odb::dbMaster::setType, master,
                              odb::dbMasterType::PAD)] >>
       -(lit("INPUT")[boost::bind(&odb::dbMaster::setType, master,
                                  odb::dbMasterType::PAD_INPUT)]) >>
       -(lit("OUTPUT")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::PAD_OUTPUT)]) >>
       -(lit("INOUT")[boost::bind(&odb::dbMaster::setType, master,
                                  odb::dbMasterType::PAD_INOUT)]) >>
       -(lit("POWER")[boost::bind(&odb::dbMaster::setType, master,
                                  odb::dbMasterType::PAD_POWER)]) >>
       -(lit("SPACER")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::PAD_SPACER)]) >>
       -(lit("AREAIO")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::PAD_AREAIO)]));

  qi::rule<std::string_view::iterator, space_type> endcap =
      (lit("ENDCAP") >
       (lit("BOTTOMEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_BOTTOMEDGE)] |
        lit("TOPEDGE")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::ENDCAP_LEF58_TOPEDGE)] |
        lit("RIGHTEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTEDGE)] |
        lit("LEFTEDGE")[boost::bind(&odb::dbMaster::setType, master,
                                    odb::dbMasterType::ENDCAP_LEF58_LEFTEDGE)] |
        lit("RIGHTBOTTOMEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_TOPEDGE)] |
        lit("RIGHTEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTEDGE)] |
        lit("LEFTEDGE")[boost::bind(&odb::dbMaster::setType, master,
                                    odb::dbMasterType::ENDCAP_LEF58_LEFTEDGE)] |
        lit("RIGHTBOTTOMEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE)] |
        lit("LEFTBOTTOMEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE)] |
        lit("RIGHTTOPEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE)] |
        lit("LEFTTOPEDGE")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE)] |
        lit("RIGHTBOTTOMCORNER")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER)] |
        lit("LEFTBOTTOMCORNER")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER)] |
        lit("RIGHTTOPCORNER")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER)] |
        lit("LEFTTOPCORNER")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER)] |
        lit("PRE")[boost::bind(&odb::dbMaster::setType, master,
                               odb::dbMasterType::ENDCAP_PRE)] |
        lit("POST")[boost::bind(&odb::dbMaster::setType, master,
                                odb::dbMasterType::ENDCAP_POST)] |
        lit("TOPLEFT")[boost::bind(&odb::dbMaster::setType, master,
                                   odb::dbMasterType::ENDCAP_TOPLEFT)] |
        lit("TOPRIGHT")[boost::bind(&odb::dbMaster::setType, master,
                                    odb::dbMasterType::ENDCAP_TOPRIGHT)] |
        lit("BOTTOMLEFT")[boost::bind(&odb::dbMaster::setType, master,
                                      odb::dbMasterType::ENDCAP_BOTTOMLEFT)] |
        lit("BOTTOMRIGHT")[boost::bind(
            &odb::dbMaster::setType, master,
            odb::dbMasterType::ENDCAP_BOTTOMRIGHT)]));

  qi::rule<std::string_view::iterator, space_type> start =
      (lit("CLASS") >> (endcap | pad | core | block | ring | cover) >>
       lit(";"));

  bool valid = qi::phrase_parse(first, last, start, space);

  return valid && first == last;
}
}  // namespace lefMacroClassType

namespace odb {

bool lefMacroClassTypeParser::parse(std::string_view s, dbMaster* master) {
  return lefMacroClassType::parse(s.begin(), s.end(), master);
}

}  // namespace odb
