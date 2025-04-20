// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "AbstractMakeWireParasitics.h"
#include "FastRoute.h"
#include "Grid.h"
#include "Net.h"
#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "sta/Clock.hh"
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

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace sta {
class SpefWriter;
}

namespace rsz {
class Resizer;
}

namespace grt {

class MakeWireParasitics : public AbstractMakeWireParasitics
{
 public:
  MakeWireParasitics(utl::Logger* logger,
                     rsz::Resizer* resizer,
                     sta::dbSta* sta,
                     odb::dbTech* tech,
                     odb::dbBlock* block,
                     GlobalRouter* grouter);
  void estimateParasitcs(odb::dbNet* net,
                         std::vector<Pin>& pins,
                         GRoute& route,
                         sta::SpefWriter* spef_writer = nullptr);
  void estimateParasitcs(odb::dbNet* net, GRoute& route) override;

  void clearParasitics() override;
  // Return GRT layer lengths in dbu's for db_net's route indexed by routing
  // layer.
  std::vector<int> routeLayerLengths(odb::dbNet* db_net) const;
  // Return the Slack of a given net
  float getNetSlack(odb::dbNet* net) override;

 private:
  using NodeRoutePtMap = std::map<RoutePt, sta::ParasiticNode*>;

  sta::Pin* staPin(Pin& pin) const;
  void makeRouteParasitics(odb::dbNet* net,
                           GRoute& route,
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
  void makeParasiticsToPins(std::vector<Pin>& pins,
                            odb::dbNet* net,
                            NodeRoutePtMap& node_map,
                            sta::Corner* corner,
                            sta::ParasiticAnalysisPt* analysis_point,
                            sta::Parasitic* parasitic);
  void makeParasiticsToPin(Pin& pin,
                           odb::dbNet* net,
                           NodeRoutePtMap& node_map,
                           sta::Corner* corner,
                           sta::ParasiticAnalysisPt* analysis_point,
                           sta::Parasitic* parasitic);
  void makePartialParasiticsToPins(std::vector<Pin>& pins,
                                   NodeRoutePtMap& node_map,
                                   sta::Corner* corner,
                                   sta::ParasiticAnalysisPt* analysis_point,
                                   sta::Parasitic* parasitic,
                                   odb::dbNet* net);
  void makePartialParasiticsToPin(Pin& pin,
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
  float getCutLayerRes(odb::dbTechLayer* cut_layer,
                       sta::Corner* corner,
                       int num_cuts = 1) const;
  double dbuToMeters(int dbu) const;

  // Variables common to all nets.
  GlobalRouter* grouter_;
  odb::dbTech* tech_;
  odb::dbBlock* block_;
  utl::Logger* logger_;
  rsz::Resizer* resizer_;
  sta::dbSta* sta_;
  sta::dbNetwork* network_;
  sta::Parasitics* parasitics_;
  sta::ArcDelayCalc* arc_delay_calc_;
  const sta::MinMax* min_max_;
  size_t resistor_id_;
};

}  // namespace grt
