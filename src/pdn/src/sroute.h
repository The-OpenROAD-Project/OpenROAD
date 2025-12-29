// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "odb/db.h"

namespace pdn {

class PdnGen;
class VoltageDomain;

class SRoute
{
 public:
  SRoute(PdnGen* pdngen, odb::dbDatabase* db, utl::Logger* logger);

  void createSrouteWires(const char* net_name,
                         const char* outer_net_name,
                         odb::dbTechLayer* layer0,
                         odb::dbTechLayer* layer1,
                         int cut_pitch_x,
                         int cut_pitch_y,
                         const std::vector<odb::dbTechViaGenerateRule*>& vias,
                         const std::vector<odb::dbTechVia*>& techvias,
                         int max_rows,
                         int max_columns,
                         const std::vector<odb::dbTechLayer*>& ongrid,
                         const std::vector<int>& metalwidths,
                         const std::vector<int>& metalspaces,
                         const std::vector<odb::dbInst*>& insts);

 private:
  void addSrouteInst(odb::dbNet* net,
                     odb::dbInst* inst,
                     const char* iterm_name,
                     const std::vector<odb::dbSBox*>& ring);
  std::vector<odb::dbSBox*> findRingShapes(odb::dbNet* net, uint32_t& Hdy);
  std::vector<VoltageDomain*> getDomains() const;

  utl::Logger* logger_;
  PdnGen* pdngen_;
  odb::dbDatabase* db_;
  std::vector<std::vector<odb::dbITerm*>> sroute_itermss_;
};

}  // namespace pdn
