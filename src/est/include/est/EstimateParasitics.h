// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "sta/Path.hh"
#include "sta/UnorderedSet.hh"
#include "utl/Logger.h"

namespace grt {
class GlobalRouter;
class IncrementalGRoute;
}  // namespace grt

namespace stt {
class SteinerTreeBuilder;
}

namespace rsz{
class SteinerTree;
}  // namespace rsz

namespace est {

using utl::Logger;

using odb::dbBlock;
using odb::dbDatabase;
using odb::dbInst;
using odb::dbMaster;
using odb::dbNet;
using odb::dbTechLayer;
using odb::Point;
using odb::Rect;

using stt::SteinerTreeBuilder;

using grt::GlobalRouter;
using grt::IncrementalGRoute;

using sta::ArcDelay;
using sta::Cell;
using sta::Corner;
using sta::dbNetwork;
using sta::dbNetworkObserver;
using sta::dbSta;
using sta::dbStaState;
using sta::DcalcAnalysisPt;
using sta::Delay;
using sta::GateTimingModel;
using sta::Instance;
using sta::InstanceSeq;
using sta::InstanceSet;
using sta::LibertyCell;
using sta::LibertyCellSeq;
using sta::LibertyCellSet;
using sta::LibertyLibrary;
using sta::LibertyLibrarySeq;
using sta::LibertyPort;
using sta::Map;
using sta::MinMax;
using sta::Net;
using sta::NetSeq;
using sta::Parasitic;
using sta::ParasiticAnalysisPt;
using sta::ParasiticNode;
using sta::Parasitics;
using sta::Pin;
using sta::PinSeq;
using sta::PinSet;
using sta::Pvt;
using sta::Required;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::SpefWriter;
using sta::TimingArc;
using sta::UnorderedSet;
using sta::Vector;
using sta::Vertex;
using sta::VertexSeq;
using sta::VertexSet;

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

class EstimateParasitics : public dbStaState
{
 public:
  EstimateParasitics();
  ~EstimateParasitics() override;
  void init(Logger* logger,
            dbDatabase* db,
            dbSta* sta,
            SteinerTreeBuilder* stt_builder,
            GlobalRouter* global_router);
  void setLayerRC(dbTechLayer* layer,
                    const Corner* corner,
                    double res,
                    double cap);
  void layerRC(dbTechLayer* layer,
               const Corner* corner,
               // Return values.
               double& res,
               double& cap) const;
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
  bool haveEstimatedParasitics() const;
  void parasiticsInvalid(const Net* net);
  void parasiticsInvalid(const dbNet* net);
  bool parasiticsValid() const;
  
 private:
  void initBlock();
  void ensureParasitics();
  void updateParasitics(bool save_guides = false);
  void ensureWireParasitic(const Pin* drvr_pin);
  void ensureWireParasitic(const Pin* drvr_pin, const Net* net);
  void estimateWireParasiticSteiner(const Pin* drvr_pin,
                                    const Net* net,
                                    SpefWriter* spef_writer);
  void makePadParasitic(const Net* net, SpefWriter* spef_writer);
  bool isPadNet(const Net* net) const;
  bool isPadPin(const Pin* pin) const;
  bool isPad(const Instance* inst) const;
  float pinCapacitance(const Pin* pin, const DcalcAnalysisPt* dcalc_ap) const;
  float totalLoad(rsz::SteinerTree* tree) const;
  float subtreeLoad(rsz::SteinerTree* tree,
                    float cap_per_micron,
                    SteinerPt pt) const;
  void parasiticNodeConnectPins(Parasitic* parasitic,
                                ParasiticNode* node,
                                rsz::SteinerTree* tree,
                                SteinerPt pt,
                                size_t& resistor_id);
  void net2Pins(const Net* net, const Pin*& pin1, const Pin*& pin2) const;
  double dbuToMeters(int dist) const;

  Logger* logger_ = nullptr;
  SteinerTreeBuilder* stt_builder_ = nullptr;
  GlobalRouter* global_router_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  IncrementalGRoute* incr_groute_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;

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
  UnorderedSet<const Net*, NetHash> parasitics_invalid_;
  const DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;
  bool incremental_parasitics_enabled_ = false;

  int dbu_ = 0;

  // constants
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

};


}  // namespace est