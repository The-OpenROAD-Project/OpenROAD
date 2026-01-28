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

#include "db_sta/SpefWriter.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/SteinerTree.h"
#include "grt/GRoute.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Hash.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Path.hh"
#include "sta/UnorderedSet.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace utl {
class CallBackHandler;
}  // namespace utl

namespace est {

using SteinerPt = int;

class NetHash
{
 public:
  size_t operator()(const sta::Net* net) const { return hashPtr(net); }
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

class EstimateParasitics : public sta::dbStaState
{
 public:
  EstimateParasitics(utl::Logger* logger,
                     utl::CallBackHandler* callback_handler,
                     odb::dbDatabase* db,
                     sta::dbSta* sta,
                     stt::SteinerTreeBuilder* stt_builder,
                     grt::GlobalRouter* global_router);
  ~EstimateParasitics() override;
  void initSteinerRenderer(
      std::unique_ptr<est::AbstractSteinerRenderer> steiner_renderer);
  void setLayerRC(odb::dbTechLayer* layer,
                  const sta::Corner* corner,
                  double res,
                  double cap);
  void layerRC(odb::dbTechLayer* layer,
               const sta::Corner* corner,
               // Return values.
               double& res,
               double& cap) const;
  void addClkLayer(odb::dbTechLayer* layer);
  void addSignalLayer(odb::dbTechLayer* layer);
  void sortClkAndSignalLayers();
  // Set the resistance and capacitance used for horizontal parasitics on signal
  // nets.
  void setHWireSignalRC(const sta::Corner* corner,
                        double res,   // ohms/meter
                        double cap);  // farads/meter
  // Set the resistance and capacitance used for vertical wires parasitics on
  // signal nets.
  void setVWireSignalRC(const sta::Corner* corner,
                        double res,   // ohms/meter
                        double cap);  // farads/meter
  // Set the resistance and capacitance used for parasitics on clock nets.
  void setHWireClkRC(const sta::Corner* corner,
                     double res,
                     double cap);  // farads/meter
  // Set the resistance and capacitance used for parasitics on clock nets.
  void setVWireClkRC(const sta::Corner* corner,
                     double res,
                     double cap);  // farads/meter
  // ohms/meter, farads/meter
  void wireSignalRC(const sta::Corner* corner,
                    // Return values.
                    double& res,
                    double& cap) const;
  double wireSignalResistance(const sta::Corner* corner) const;
  double wireSignalHResistance(const sta::Corner* corner) const;
  double wireSignalVResistance(const sta::Corner* corner) const;
  double wireClkResistance(const sta::Corner* corner) const;
  double wireClkHResistance(const sta::Corner* corner) const;
  double wireClkVResistance(const sta::Corner* corner) const;
  // farads/meter
  double wireSignalCapacitance(const sta::Corner* corner) const;
  double wireSignalHCapacitance(const sta::Corner* corner) const;
  double wireSignalVCapacitance(const sta::Corner* corner) const;
  double wireClkCapacitance(const sta::Corner* corner) const;
  double wireClkHCapacitance(const sta::Corner* corner) const;
  double wireClkVCapacitance(const sta::Corner* corner) const;
  void estimateParasitics(ParasiticsSrc src);
  void estimateParasitics(ParasiticsSrc src,
                          std::map<sta::Corner*, std::ostream*>& spef_streams_);
  void estimateWireParasitics(sta::SpefWriter* spef_writer = nullptr);
  void estimateWireParasitic(const sta::Net* net,
                             sta::SpefWriter* spef_writer = nullptr);
  void estimateWireParasitic(const sta::Pin* drvr_pin,
                             const sta::Net* net,
                             sta::SpefWriter* spef_writer = nullptr);
  void makeWireParasitic(sta::Net* net,
                         sta::Pin* drvr_pin,
                         sta::Pin* load_pin,
                         double wire_length,  // meters
                         const sta::Corner* corner,
                         sta::Parasitics* parasitics);
  bool haveEstimatedParasitics() const;
  void parasiticsInvalid(const sta::Net* net);
  void parasiticsInvalid(const odb::dbNet* net);
  void eraseParasitics(const sta::Net* net);
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

  void removeNetFromParasiticsInvalid(sta::Net* net)
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
  SteinerTree* makeSteinerTree(const sta::Pin* drvr_pin);
  void updateParasitics(bool save_guides = false);
  void ensureWireParasitic(const sta::Pin* drvr_pin);
  void ensureWireParasitic(const sta::Pin* drvr_pin, const sta::Net* net);
  void highlightSteiner(const sta::Pin* drvr);

  sta::dbNetwork* getDbNetwork() { return db_network_; }
  odb::dbBlock* getBlock() { return block_; }
  grt::GlobalRouter* getGlobalRouter() { return global_router_; }
  grt::IncrementalGRoute* getIncrementalGRT() { return incr_groute_; }
  void setIncrementalGRT(grt::IncrementalGRoute* incr_groute)
  {
    incr_groute_ = incr_groute;
  }
  void setDbCbkOwner(odb::dbBlock* block);
  void removeDbCbkOwner();

  void initBlock();

  utl::Logger* getLogger() { return logger_; }

 private:
  void ensureParasitics();
  void estimateWireParasiticSteiner(const sta::Pin* drvr_pin,
                                    const sta::Net* net,
                                    sta::SpefWriter* spef_writer);
  void makePadParasitic(const sta::Net* net, sta::SpefWriter* spef_writer);
  bool isPadNet(const sta::Net* net) const;
  bool isPadPin(const sta::Pin* pin) const;
  bool isPad(const sta::Instance* inst) const;
  float pinCapacitance(const sta::Pin* pin,
                       const sta::DcalcAnalysisPt* dcalc_ap) const;
  odb::dbTechLayer* getPinLayer(const sta::Pin* pin);
  double computeAverageCutResistance(sta::Corner* corner);
  void parasiticNodeConnectPins(sta::Parasitic* parasitic,
                                sta::ParasiticNode* node,
                                SteinerTree* tree,
                                SteinerPt pt,
                                size_t& resistor_id,
                                sta::Corner* corner,
                                std::set<const sta::Pin*>& connected_pins,
                                bool is_clk);
  void net2Pins(const sta::Net* net,
                const sta::Pin*& pin1,
                const sta::Pin*& pin2) const;
  double dbuToMeters(int dist) const;

  utl::Logger* logger_ = nullptr;
  std::unique_ptr<EstimateParasiticsCallBack> estimate_parasitics_cbk_;
  stt::SteinerTreeBuilder* stt_builder_ = nullptr;
  grt::GlobalRouter* global_router_ = nullptr;
  grt::IncrementalGRoute* incr_groute_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
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
  const sta::DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;

  sta::UnorderedSet<const sta::Net*, NetHash> parasitics_invalid_;

  std::unique_ptr<AbstractSteinerRenderer> steiner_renderer_;

  int dbu_ = 0;

  bool incremental_parasitics_enabled_ = false;

  // constants
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();
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
