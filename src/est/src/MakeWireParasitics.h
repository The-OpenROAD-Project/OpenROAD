// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "db_sta/dbSta.hh"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "grt/PinGridLocation.h"
#include "grt/RoutePt.h"
#include "odb/db.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Set.hh"

namespace sta {
class Net;
class dbNetwork;
class Parasitics;
class Parasitic;
class Corner;
class OperatingConditions;
class ParasiticAnalysisPt;
class Units;
}  // namespace sta

namespace utl {
class Logger;
}

namespace sta {
class SpefWriter;
}

namespace est {
class EstimateParasitics;

class MakeWireParasitics
{
 public:
  MakeWireParasitics(utl::Logger* logger,
                     est::EstimateParasitics* estimate_parasitics,
                     sta::dbSta* sta,
                     odb::dbTech* tech,
                     odb::dbBlock* block,
                     grt::GlobalRouter* grouter);
  void estimateParasitics(odb::dbNet* net,
                          grt::GRoute& route,
                          sta::SpefWriter* spef_writer);
  void estimateParasitics(odb::dbNet* net, grt::GRoute& route);

  void clearParasitics();

  // Return the Slack of a given net
  float getNetSlack(odb::dbNet* net);

  using NodeRoutePtMap = std::map<grt::RoutePt, sta::ParasiticNode*>;

 private:
  sta::Pin* staPin(odb::dbBTerm* bterm) const;
  sta::Pin* staPin(odb::dbITerm* iterm) const;
  void makeRouteParasitics(odb::dbNet* net,
                           grt::GRoute& route,
                           sta::Net* sta_net,
                           sta::Corner* corner,
                           sta::ParasiticAnalysisPt* analysis_point,
                           sta::Parasitic* parasitic,
                           NodeRoutePtMap& node_map);
  sta::ParasiticNode* ensureParasiticNode(int x,
                                          int y,
                                          int layer,
                                          NodeRoutePtMap& node_map,
                                          sta::Parasitic* parasitic,
                                          sta::Net* net) const;
  void makeParasiticsToPins(odb::dbNet* net,
                            std::vector<grt::PinGridLocation>& pin_grid_locs,
                            NodeRoutePtMap& node_map,
                            sta::Corner* corner,
                            sta::ParasiticAnalysisPt* analysis_point,
                            sta::Parasitic* parasitic);
  void makeParasiticsToPin(odb::dbNet* net,
                           grt::PinGridLocation& pin_loc,
                           NodeRoutePtMap& node_map,
                           sta::Corner* corner,
                           sta::ParasiticAnalysisPt* analysis_point,
                           sta::Parasitic* parasitic);
  void makePartialParasiticsToPins(
      std::vector<grt::PinGridLocation>& pin_grid_locs,
      NodeRoutePtMap& node_map,
      sta::Corner* corner,
      sta::ParasiticAnalysisPt* analysis_point,
      sta::Parasitic* parasitic,
      odb::dbNet* net);
  void makePartialParasiticsToPin(grt::PinGridLocation& pin_loc,
                                  NodeRoutePtMap& node_map,
                                  sta::Corner* corner,
                                  sta::ParasiticAnalysisPt* analysis_point,
                                  sta::Parasitic* parasitic,
                                  odb::dbNet* net);
  void layerRC(int wire_length_dbu,
               int layer,
               sta::Corner* corner,
               // Return values.
               float& res,
               float& cap) const;
  void layerRC(int wire_length_dbu,
               int layer,
               sta::Corner* corner,
               odb::dbNet* net,
               // Return values.
               float& res,
               float& cap) const;
  float getCutLayerRes(odb::dbTechLayer* cut_layer,
                       sta::Corner* corner,
                       int num_cuts = 1) const;
  double dbuToMeters(int dbu) const;

  // Variables common to all nets.
  grt::GlobalRouter* global_router_;
  est::EstimateParasitics* estimate_parasitics_;
  odb::dbTech* tech_;
  odb::dbBlock* block_;
  utl::Logger* logger_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  sta::Parasitics* parasitics_;
  sta::ArcDelayCalc* arc_delay_calc_;
  const sta::MinMax* min_max_;
  size_t resistor_id_;
};

}  // namespace est
