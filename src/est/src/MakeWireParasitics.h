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

namespace sta {
class Net;
class dbNetwork;
class Parasitics;
class Parasitic;
class Scene;
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
  void makeRouteParasitics(sta::Parasitics* parasitics,
                           odb::dbNet* net,
                           grt::GRoute& route,
                           sta::Net* sta_net,
                           sta::Scene* corner,
                           sta::Parasitic* parasitic,
                           NodeRoutePtMap& node_map);
  sta::ParasiticNode* ensureParasiticNode(int x,
                                          int y,
                                          int layer,
                                          NodeRoutePtMap& node_map,
                                          sta::Parasitics* parasitics,
                                          sta::Parasitic* parasitic,
                                          sta::Net* net) const;
  void makeParasiticsToPins(sta::Parasitics* parasitics,
                            odb::dbNet* net,
                            std::vector<grt::PinGridLocation>& pin_grid_locs,
                            NodeRoutePtMap& node_map,
                            sta::Scene* corner,
                            sta::Parasitic* parasitic);
  void makeParasiticsToPin(sta::Parasitics* parasitics,
                           odb::dbNet* net,
                           grt::PinGridLocation& pin_loc,
                           NodeRoutePtMap& node_map,
                           sta::Scene* corner,
                           sta::Parasitic* parasitic);
  void makePartialParasiticsToPins(
      sta::Parasitics* parasitics,
      std::vector<grt::PinGridLocation>& pin_grid_locs,
      NodeRoutePtMap& node_map,
      sta::Scene* corner,
      sta::Parasitic* parasitic,
      odb::dbNet* net);
  void makePartialParasiticsToPin(sta::Parasitics* parasitics,
                                  grt::PinGridLocation& pin_loc,
                                  NodeRoutePtMap& node_map,
                                  sta::Scene* corner,
                                  sta::Parasitic* parasitic,
                                  odb::dbNet* net);
  void layerRC(int wire_length_dbu,
               int layer,
               sta::Scene* corner,
               // Return values.
               float& res,
               float& cap) const;
  void layerRC(int wire_length_dbu,
               int layer,
               sta::Scene* corner,
               odb::dbNet* net,
               // Return values.
               float& res,
               float& cap) const;
  float getCutLayerRes(odb::dbTechLayer* cut_layer,
                       sta::Scene* corner,
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
  sta::ArcDelayCalc* arc_delay_calc_;
  const sta::MinMax* min_max_;
  size_t resistor_id_;
};

}  // namespace est
