// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/SteinerTree.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/UnorderedSet.hh"
#include "utl/Logger.h"

namespace grt {
class GlobalRouter;
class IncrementalGRoute;
}  // namespace grt

namespace stt {
class SteinerTreeBuilder;
}  // namespace stt

namespace utl {
class CallBackHandler;
}  // namespace utl

namespace est {

using utl::Logger;

using stt::SteinerTreeBuilder;

using grt::GlobalRouter;
using grt::IncrementalGRoute;

using sta::ArcDelay;
using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::dbStaState;
using sta::DcalcAnalysisPt;
using sta::Delay;
using sta::Instance;
using sta::InstanceSeq;
using sta::LibertyCellSeq;
using sta::LibertyCellSet;
using sta::LibertyLibrarySeq;
using sta::MinMax;
using sta::Net;
using sta::NetSeq;
using sta::Parasitic;
using sta::ParasiticNode;
using sta::Parasitics;
using sta::Pin;
using sta::PinSeq;
using sta::Required;
using sta::Slack;
using sta::Slew;
using sta::SpefWriter;
using sta::UnorderedSet;
using sta::VertexSeq;

using SteinerPt = int;

class NetHash
{
 public:
  size_t operator()(const Net* net) const { return hashPtr(net); }
};

enum class ParasiticsSrc
{
  none,
  placement,
  global_routing,
  detailed_routing
};

struct ParasiticsResistance
{
  double h_res;
  double v_res;
};

struct ParasiticsCapacitance
{
  double h_cap;
  double v_cap;
};

class AbstractSteinerRenderer;
class OdbCallBack;
class EstimateParasiticsCallBack;

class EstimateParasitics : public dbStaState
{
 public:
  EstimateParasitics(Logger* logger,
                     utl::CallBackHandler* callback_handler,
                     odb::dbDatabase* db,
                     dbSta* sta,
                     SteinerTreeBuilder* stt_builder,
                     GlobalRouter* global_router);
  ~EstimateParasitics() override;
  void initSteinerRenderer(
      std::unique_ptr<est::AbstractSteinerRenderer> steiner_renderer);
  void setLayerRC(odb::dbTechLayer* layer,
                  const Corner* corner,
                  double res,
                  double cap);
  void layerRC(odb::dbTechLayer* layer,
               const Corner* corner,
               // Return values.
               double& res,
               double& cap) const;
  void addClkLayer(odb::dbTechLayer* layer);
  void addSignalLayer(odb::dbTechLayer* layer);
  void sortClkAndSignalLayers();
  // Set the resistance and capacitance used for horizontal parasitics on signal
  // nets.
  void setHWireSignalRC(const Corner* corner,
                        double res,   // ohms/meter
                        double cap);  // farads/meter
  // Set the resistance and capacitance used for vertical wires parasitics on
  // signal nets.
  void setVWireSignalRC(const Corner* corner,
                        double res,   // ohms/meter
                        double cap);  // farads/meter
  // Set the resistance and capacitance used for parasitics on clock nets.
  void setHWireClkRC(const Corner* corner,
                     double res,
                     double cap);  // farads/meter
  // Set the resistance and capacitance used for parasitics on clock nets.
  void setVWireClkRC(const Corner* corner,
                     double res,
                     double cap);  // farads/meter
  // ohms/meter, farads/meter
  void wireSignalRC(const Corner* corner,
                    // Return values.
                    double& res,
                    double& cap) const;
  double wireSignalResistance(const Corner* corner) const;
  double wireSignalHResistance(const Corner* corner) const;
  double wireSignalVResistance(const Corner* corner) const;
  double wireClkResistance(const Corner* corner) const;
  double wireClkHResistance(const Corner* corner) const;
  double wireClkVResistance(const Corner* corner) const;
  // farads/meter
  double wireSignalCapacitance(const Corner* corner) const;
  double wireSignalHCapacitance(const Corner* corner) const;
  double wireSignalVCapacitance(const Corner* corner) const;
  double wireClkCapacitance(const Corner* corner) const;
  double wireClkHCapacitance(const Corner* corner) const;
  double wireClkVCapacitance(const Corner* corner) const;
  void estimateParasitics(ParasiticsSrc src);
  void estimateParasitics(ParasiticsSrc src,
                          std::map<Corner*, std::ostream*>& spef_streams_);
  void estimateWireParasitics(SpefWriter* spef_writer = nullptr);
  void estimateWireParasitic(const Net* net, SpefWriter* spef_writer = nullptr);
  void estimateWireParasitic(const Pin* drvr_pin,
                             const Net* net,
                             SpefWriter* spef_writer = nullptr);
  void makeWireParasitic(Net* net,
                         Pin* drvr_pin,
                         Pin* load_pin,
                         double wire_length,  // meters
                         const Corner* corner,
                         Parasitics* parasitics);
  bool haveEstimatedParasitics() const;
  void parasiticsInvalid(const Net* net);
  void parasiticsInvalid(const odb::dbNet* net);
  void eraseParasitics(const Net* net);
  bool parasiticsValid() const;

  ParasiticsSrc getParasiticsSrc() { return parasitics_src_; }
  void setParasiticsSrc(ParasiticsSrc src);

  bool isIncrementalParasiticsEnabled() const
  {
    return incremental_parasitics_enabled_;
  }
  void setIncrementalParasiticsEnabled(bool enabled)
  {
    incremental_parasitics_enabled_ = enabled;
  }

  void removeNetFromParasiticsInvalid(Net* net)
  {
    parasitics_invalid_.erase(net);
  }

  bool hasParasiticsInvalid() const { return !parasitics_invalid_.empty(); }

  // Functions to estimate RC from global routing results
  void estimateGlobalRouteRC(sta::SpefWriter* spef_writer = nullptr);
  void estimateGlobalRouteRC(odb::dbNet* db_net);
  void estimateGlobalRouteParasitics(odb::dbNet* net, grt::GRoute& route);
  void clearParasitics();

  ////////////////////////////////////////////////////////////////
  // Returns nullptr if net has less than 2 pins or any pin is not placed.
  SteinerTree* makeSteinerTree(odb::Point drvr_location,
                               const std::vector<odb::Point>& sink_locations);
  SteinerTree* makeSteinerTree(const Pin* drvr_pin);
  void updateParasitics(bool save_guides = false);
  void ensureWireParasitic(const Pin* drvr_pin);
  void ensureWireParasitic(const Pin* drvr_pin, const Net* net);
  void highlightSteiner(const Pin* drvr);

  dbNetwork* getDbNetwork() { return db_network_; }
  odb::dbBlock* getBlock() { return block_; }
  GlobalRouter* getGlobalRouter() { return global_router_; }
  IncrementalGRoute* getIncrementalGRT() { return incr_groute_; }
  void setIncrementalGRT(IncrementalGRoute* incr_groute)
  {
    incr_groute_ = incr_groute;
  }
  void setDbCbkOwner(odb::dbBlock* block);
  void removeDbCbkOwner();

  void initBlock();

  Logger* getLogger() { return logger_; }

 private:
  void ensureParasitics();
  void estimateWireParasiticSteiner(const Pin* drvr_pin,
                                    const Net* net,
                                    SpefWriter* spef_writer);
  void makePadParasitic(const Net* net, SpefWriter* spef_writer);
  bool isPadNet(const Net* net) const;
  bool isPadPin(const Pin* pin) const;
  bool isPad(const Instance* inst) const;
  float pinCapacitance(const Pin* pin, const DcalcAnalysisPt* dcalc_ap) const;
  odb::dbTechLayer* getPinLayer(const Pin* pin);
  double computeAverageCutResistance(Corner* corner);
  void parasiticNodeConnectPins(Parasitic* parasitic,
                                ParasiticNode* node,
                                SteinerTree* tree,
                                SteinerPt pt,
                                size_t& resistor_id,
                                Corner* corner,
                                std::set<const Pin*>& connected_pins,
                                bool is_clk);
  void net2Pins(const Net* net, const Pin*& pin1, const Pin*& pin2) const;
  double dbuToMeters(int dist) const;

  Logger* logger_ = nullptr;
  std::unique_ptr<EstimateParasiticsCallBack> estimate_parasitics_cbk_;
  SteinerTreeBuilder* stt_builder_ = nullptr;
  GlobalRouter* global_router_ = nullptr;
  IncrementalGRoute* incr_groute_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  std::unique_ptr<OdbCallBack> db_cbk_;

  std::vector<odb::dbTechLayer*> signal_layers_;
  std::vector<odb::dbTechLayer*> clk_layers_;
  // Layer RC per wire length indexed by layer->getNumber(), corner->index
  std::vector<std::vector<double>> layer_res_;  // ohms/meter
  std::vector<std::vector<double>> layer_cap_;  // Farads/meter
  // Signal wire RC indexed by corner->index
  std::vector<ParasiticsResistance> wire_signal_res_;   // ohms/metre
  std::vector<ParasiticsCapacitance> wire_signal_cap_;  // Farads/meter
  // Clock wire RC.
  std::vector<ParasiticsResistance> wire_clk_res_;   // ohms/metre
  std::vector<ParasiticsCapacitance> wire_clk_cap_;  // Farads/meter

  ParasiticsSrc parasitics_src_ = ParasiticsSrc::none;
  const DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;

  UnorderedSet<const Net*, NetHash> parasitics_invalid_;

  std::unique_ptr<AbstractSteinerRenderer> steiner_renderer_;

  int dbu_ = 0;

  bool incremental_parasitics_enabled_ = false;

  // constants
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
};

class IncrementalParasiticsGuard
{
 public:
  IncrementalParasiticsGuard(est::EstimateParasitics* estimate_parasitics);
  ~IncrementalParasiticsGuard();

  // calls estimate_parasitics_->updateParasitics()
  void update();

 private:
  est::EstimateParasitics* estimate_parasitics_;
  bool need_unregister_;
};

}  // namespace est
