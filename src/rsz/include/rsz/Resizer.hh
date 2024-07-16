/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <optional>
#include <string>

#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
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

namespace rsz {

using std::array;
using std::string;
using std::vector;

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
using sta::TimingArc;
using sta::UnorderedSet;
using sta::Vector;
using sta::Vertex;
using sta::VertexSeq;
using sta::VertexSet;

using LibertyPortTuple = std::tuple<LibertyPort*, LibertyPort*>;
using InstanceTuple = std::tuple<Instance*, Instance*>;
using InputSlews = std::array<Slew, RiseFall::index_count>;

class AbstractSteinerRenderer;
class SteinerTree;
using SteinerPt = int;

class BufferedNet;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;

using PinPtr = const sta::Pin*;
using PinVector = std::vector<PinPtr>;

class RecoverPower;
class RepairDesign;
class RepairSetup;
class RepairHold;

class NetHash
{
 public:
  size_t operator()(const Net* net) const { return hashPtr(net); }
};

using CellTargetLoadMap = Map<LibertyCell*, float>;
using TgtSlews = array<Slew, RiseFall::index_count>;

enum class ParasiticsSrc
{
  none,
  placement,
  global_routing
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

class Resizer : public dbStaState
{
 public:
  Resizer();
  ~Resizer() override;
  void init(Logger* logger,
            dbDatabase* db,
            dbSta* sta,
            SteinerTreeBuilder* stt_builder,
            GlobalRouter* global_router,
            dpl::Opendp* opendp,
            std::unique_ptr<AbstractSteinerRenderer> steiner_renderer);
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
  void estimateWireParasitics();
  void estimateWireParasitic(const Net* net);
  void estimateWireParasitic(const Pin* drvr_pin, const Net* net);
  bool haveEstimatedParasitics() const;
  void parasiticsInvalid(const Net* net);
  void parasiticsInvalid(const dbNet* net);
  bool parasiticsValid() const;

  // Core area (meters).
  double coreArea() const;
  // 0.0 - 1.0 (100%) of core size.
  double utilization();
  // Maximum utilizable area (core area * utilization)
  double maxArea() const;

  void setDontUse(LibertyCell* cell, bool dont_use);
  bool dontUse(LibertyCell* cell);
  void setDontTouch(const Instance* inst, bool dont_touch);
  bool dontTouch(const Instance* inst);
  void setDontTouch(const Net* net, bool dont_touch);
  bool dontTouch(const Net* net);

  void setMaxUtilization(double max_utilization);
  // Remove all or selected buffers from the netlist.
  void removeBuffers(InstanceSeq insts);
  void bufferInputs();
  void bufferOutputs();

  // Balance the usage of hybrid rows
  void balanceRowUsage();

  // Resize drvr_pin instance to target slew.
  void resizeDrvrToTargetSlew(const Pin* drvr_pin);
  // Accessor for debugging.
  Slew targetSlew(const RiseFall* rf);
  // Accessor for debugging.
  float targetLoadCap(LibertyCell* cell);

  ////////////////////////////////////////////////////////////////
  void repairSetup(double setup_margin,
                   double repair_tns_end_percent,
                   int max_passes,
                   bool verbose,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_buffer_removal);
  // For testing.
  void repairSetup(const Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin* drvr_pin);

  ////////////////////////////////////////////////////////////////

  void repairHold(double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  // Max buffer count as percent of design instance count.
                  float max_buffer_percent,
                  int max_passes,
                  bool verbose);
  void repairHold(const Pin* end_pin,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  float max_buffer_percent,
                  int max_passes);
  int holdBufferCount() const;

  ////////////////////////////////////////////////////////////////
  void recoverPower(float recover_power_percent);

  ////////////////////////////////////////////////////////////////
  // Area of the design in meter^2.
  double designArea();
  // Increment design_area
  void designAreaIncr(float delta);
  // Caller owns return value.
  NetSeq* findFloatingNets();
  PinSet* findFloatingPins();
  void repairTieFanout(LibertyPort* tie_port,
                       double separation,  // meters
                       bool verbose);
  void bufferWireDelay(LibertyCell* buffer_cell,
                       double wire_length,  // meters
                       Delay& delay,
                       Slew& slew);
  void setDebugPin(const Pin* pin);
  void setWorstSlackNetsPercent(float);

  ////////////////////////////////////////////////////////////////

  // Repair long wires, max fanout violations.
  void repairDesign(
      double max_wire_length,  // max_wire_length zero for none (meters)
      double slew_margin,      // 0.0-1.0
      double cap_margin,       // 0.0-1.0
      bool verbose);
  int repairDesignBufferCount() const;
  // for debugging
  void repairNet(Net* net,
                 double max_wire_length,  // meters
                 double slew_margin,
                 double cap_margin);

  // Repair long wires from clock input pins to clock tree root buffer
  // because CTS ignores the issue.
  // no max_fanout/max_cap checks.
  // Use max_wire_length zero for none (meters)
  void repairClkNets(
      double max_wire_length);  // max_wire_length zero for none (meters)
  // Clone inverters next to the registers they drive to remove them
  // from the clock network.
  // yosys is too stupid to use the inverted clock registers
  // and TritonCTS is too stupid to balance clock networks with inverters.
  void repairClkInverters();

  void reportLongWires(int count, int digits);
  // Find the max wire length before it is faster to split the wire
  // in half with a buffer (in meters).
  double findMaxWireLength();
  double findMaxWireLength(LibertyCell* buffer_cell, const Corner* corner);
  double findMaxWireLength(LibertyPort* drvr_port, const Corner* corner);
  // Longest driver to load wire (in meters).
  double maxLoadManhattenDistance(const Net* net);

  ////////////////////////////////////////////////////////////////
  // API for timing driven placement.
  // Each pass (findResizeSlacks)
  //  estiimate parasitics
  //  repair design
  //  save slacks
  //  remove inserted buffers
  //  restore resized gates
  // resizeSlackPreamble must be called before the first findResizeSlacks.
  void resizeSlackPreamble();
  void findResizeSlacks();
  // Return nets with worst slack.
  NetSeq& resizeWorstSlackNets();
  // Return net slack, if any (indicated by the bool).
  std::optional<Slack> resizeNetSlack(const Net* net);
  // db flavor
  vector<dbNet*> resizeWorstSlackDbNets();
  std::optional<Slack> resizeNetSlack(const dbNet* db_net);

  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  PinSet findFaninFanouts(PinSet& end_pins);
  PinSet findFanins(PinSet& end_pins);

  ////////////////////////////////////////////////////////////////
  void highlightSteiner(const Pin* drvr);

  dbNetwork* getDbNetwork() { return db_network_; }
  ParasiticsSrc getParasiticsSrc() { return parasitics_src_; }
  dbBlock* getDbBlock() { return block_; };
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;
  void makeEquivCells();

 protected:
  void init();
  void initBlock();
  void initDesignArea();
  void ensureLevelDrvrVertices();
  Instance* bufferInput(const Pin* top_pin, LibertyCell* buffer_cell);
  void bufferOutput(const Pin* top_pin, LibertyCell* buffer_cell);
  bool hasTristateOrDontTouchDriver(const Net* net);
  bool isTristateDriver(const Pin* pin);
  void checkLibertyForAllCorners();
  void findBuffers();
  bool isLinkCell(LibertyCell* cell);
  void findTargetLoads();
  void balanceBin(const vector<odb::dbInst*>& bin);

  //==============================
  // APIs for gate cloning
  LibertyCell* halfDrivingPowerCell(Instance* inst);
  LibertyCell* halfDrivingPowerCell(LibertyCell* cell);
  LibertyCell* closestDriver(LibertyCell* cell,
                             LibertyCellSeq* candidates,
                             float scale);
  std::vector<sta::LibertyPort*> libraryPins(Instance* inst) const;
  std::vector<sta::LibertyPort*> libraryPins(LibertyCell* cell) const;
  bool isSingleOutputCombinational(Instance* inst) const;
  bool isSingleOutputCombinational(LibertyCell* cell) const;
  bool isCombinational(LibertyCell* cell) const;
  std::vector<sta::LibertyPort*> libraryOutputPins(LibertyCell* cell) const;
  float maxLoad(Cell* cell);
  //==============================
  float findTargetLoad(LibertyCell* cell);
  float findTargetLoad(LibertyCell* cell,
                       TimingArc* arc,
                       Slew in_slew,
                       Slew out_slew);
  Slew gateSlewDiff(LibertyCell* cell,
                    TimingArc* arc,
                    GateTimingModel* model,
                    Slew in_slew,
                    float load_cap,
                    Slew out_slew);
  void findBufferTargetSlews();
  void findBufferTargetSlews(LibertyCell* buffer,
                             const Pvt* pvt,
                             // Return values.
                             Slew slews[],
                             int counts[]);
  bool hasMultipleOutputs(const Instance* inst);

  void resizePreamble();
  // Resize drvr_pin instance to target slew.
  // Return 1 if resized.
  int resizeToTargetSlew(const Pin* drvr_pin);

  ////////////////////////////////////////////////////////////////

  void findLongWires(VertexSeq& drvrs);
  int findMaxSteinerDist(Vertex* drvr, const Corner* corner);
  float driveResistance(const Pin* drvr_pin);
  float bufferDriveResistance(const LibertyCell* buffer) const;
  // Max distance from driver to load (in dbu).
  int maxLoadManhattenDistance(Vertex* drvr);

  double findMaxWireLength1();
  float portFanoutLoad(LibertyPort* port) const;
  float portCapacitance(LibertyPort* input, const Corner* corner) const;
  float pinCapacitance(const Pin* pin, const DcalcAnalysisPt* dcalc_ap) const;
  void swapPins(Instance* inst,
                LibertyPort* port1,
                LibertyPort* port2,
                bool journal);
  void findSwapPinCandidate(LibertyPort* input_port,
                            LibertyPort* drvr_port,
                            const sta::LibertyPortSet& equiv_ports,
                            float load_cap,
                            const DcalcAnalysisPt* dcalc_ap,
                            // Return value
                            LibertyPort** swap_port);
  void gateDelays(const LibertyPort* drvr_port,
                  float load_cap,
                  const DcalcAnalysisPt* dcalc_ap,
                  // Return values.
                  ArcDelay delays[RiseFall::index_count],
                  Slew slews[RiseFall::index_count]);
  void gateDelays(const LibertyPort* drvr_port,
                  float load_cap,
                  const Slew in_slews[RiseFall::index_count],
                  const DcalcAnalysisPt* dcalc_ap,
                  // Return values.
                  ArcDelay delays[RiseFall::index_count],
                  Slew out_slews[RiseFall::index_count]);
  ArcDelay gateDelay(const LibertyPort* drvr_port,
                     float load_cap,
                     const DcalcAnalysisPt* dcalc_ap);
  ArcDelay gateDelay(const LibertyPort* drvr_port,
                     const RiseFall* rf,
                     float load_cap,
                     const DcalcAnalysisPt* dcalc_ap);
  float bufferDelay(LibertyCell* buffer_cell,
                    float load_cap,
                    const DcalcAnalysisPt* dcalc_ap);
  float bufferDelay(LibertyCell* buffer_cell,
                    const RiseFall* rf,
                    float load_cap,
                    const DcalcAnalysisPt* dcalc_ap);
  void bufferDelays(LibertyCell* buffer_cell,
                    float load_cap,
                    const DcalcAnalysisPt* dcalc_ap,
                    // Return values.
                    ArcDelay delays[RiseFall::index_count],
                    Slew slews[RiseFall::index_count]);
  void cellWireDelay(LibertyPort* drvr_port,
                     LibertyPort* load_port,
                     double wire_length,  // meters
                     // Return values.
                     Delay& delay,
                     Slew& slew);
  void makeWireParasitic(Net* net,
                         Pin* drvr_pin,
                         Pin* load_pin,
                         double wire_length,  // meters
                         const Corner* corner,
                         Parasitics* parasitics);
  string makeUniqueNetName();
  Net* makeUniqueNet();
  string makeUniqueInstName(const char* base_name);
  string makeUniqueInstName(const char* base_name, bool underscore);
  bool overMaxArea();
  bool bufferBetweenPorts(Instance* buffer);
  bool hasPort(const Net* net);
  Point location(Instance* inst);
  double area(dbMaster* master);
  double area(Cell* cell);
  double splitWireDelayDiff(double wire_length, LibertyCell* buffer_cell);
  double maxSlewWireDiff(LibertyPort* drvr_port,
                         LibertyPort* load_port,
                         double wire_length,
                         double max_slew);
  void findCellInstances(LibertyCell* cell,
                         // Return value.
                         InstanceSeq& insts);
  void findLoads(Pin* drvr_pin, PinSeq& loads);
  bool isFuncOneZero(const Pin* drvr_pin);
  bool hasPins(Net* net);
  void getPins(Net* net, PinVector& pins) const;
  void getPins(Instance* inst, PinVector& pins) const;
  Point tieLocation(const Pin* load, int separation);
  bool hasFanout(Vertex* drvr);
  InstanceSeq findClkInverters();
  void cloneClkInverter(Instance* inv);

  void incrementalParasiticsBegin();
  void incrementalParasiticsEnd();
  void ensureParasitics();
  void updateParasitics(bool save_guides = false);
  void ensureWireParasitic(const Pin* drvr_pin);
  void ensureWireParasitic(const Pin* drvr_pin, const Net* net);
  void estimateWireParasiticSteiner(const Pin* drvr_pin, const Net* net);
  float totalLoad(SteinerTree* tree) const;
  float subtreeLoad(SteinerTree* tree,
                    float cap_per_micron,
                    SteinerPt pt) const;
  void makePadParasitic(const Net* net);
  bool isPadNet(const Net* net) const;
  bool isPadPin(const Pin* pin) const;
  bool isPad(const Instance* inst) const;
  void net2Pins(const Net* net, const Pin*& pin1, const Pin*& pin2) const;
  void parasiticNodeConnectPins(Parasitic* parasitic,
                                ParasiticNode* node,
                                SteinerTree* tree,
                                SteinerPt pt,
                                size_t& resistor_id);

  bool replaceCell(Instance* inst,
                   const LibertyCell* replacement,
                   bool journal);

  void findResizeSlacks1();
  bool removeBuffer(Instance* buffer, bool honorDontTouchFixed = true);
  Instance* makeInstance(LibertyCell* cell,
                         const char* name,
                         Instance* parent,
                         const Point& loc);
  Instance* makeBuffer(LibertyCell* cell,
                       const char* name,
                       Instance* parent,
                       const Point& loc);
  void setLocation(dbInst* db_inst, const Point& pt);
  LibertyCell* findTargetCell(LibertyCell* cell,
                              float load_cap,
                              bool revisiting_inst);
  // Returns nullptr if net has less than 2 pins or any pin is not placed.
  SteinerTree* makeSteinerTree(const Pin* drvr_pin);
  BufferedNetPtr makeBufferedNet(const Pin* drvr_pin, const Corner* corner);
  BufferedNetPtr makeBufferedNetSteiner(const Pin* drvr_pin,
                                        const Corner* corner);
  BufferedNetPtr makeBufferedNetGroute(const Pin* drvr_pin,
                                       const Corner* corner);
  float bufferSlew(LibertyCell* buffer_cell,
                   float load_cap,
                   const DcalcAnalysisPt* dcalc_ap);
  float maxInputSlew(const LibertyPort* input, const Corner* corner) const;
  void checkLoadSlews(const Pin* drvr_pin,
                      double slew_margin,
                      // Return values.
                      Slew& slew,
                      float& limit,
                      float& slack,
                      const Corner*& corner);
  void warnBufferMovedIntoCore();
  bool isLogicStdCell(const Instance* inst);
  void invalidateParasitics(const Pin* pin, const Net* net);
  ////////////////////////////////////////////////////////////////
  // Jounalling support for checkpointing and backing out changes
  // during repair timing.
  void journalBegin();
  void journalEnd();
  void journalRestore(int& resize_count,
                      int& inserted_buffer_count,
                      int& cloned_gate_count);
  void journalUndoGateCloning(int& cloned_gate_count);
  void journalSwapPins(Instance* inst, LibertyPort* port1, LibertyPort* port2);
  void journalInstReplaceCellBefore(Instance* inst);
  void journalMakeBuffer(Instance* buffer);
  Instance* journalCloneInstance(LibertyCell* cell,
                                 const char* name,
                                 Instance* original_inst,
                                 Instance* parent,
                                 const Point& loc);
  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  VertexSet findFaninFanouts(VertexSet& ends);
  VertexSet findFaninRoots(VertexSet& ends);
  VertexSet findFanouts(VertexSet& reg_outs);
  bool isRegOutput(Vertex* vertex);
  bool isRegister(Vertex* vertex);
  ////////////////////////////////////////////////////////////////

  Logger* logger() const { return logger_; }

  // Components
  RecoverPower* recover_power_;
  RepairDesign* repair_design_;
  RepairSetup* repair_setup_;
  RepairHold* repair_hold_;
  std::unique_ptr<AbstractSteinerRenderer> steiner_renderer_;

  // Layer RC per wire length indexed by layer->getNumber(), corner->index
  vector<vector<double>> layer_res_;  // ohms/meter
  vector<vector<double>> layer_cap_;  // Farads/meter
  // Signal wire RC indexed by corner->index
  vector<ParasiticsResistance> wire_signal_res_;   // ohms/metre
  vector<ParasiticsCapacitance> wire_signal_cap_;  // Farads/meter
  // Clock wire RC.
  vector<ParasiticsResistance> wire_clk_res_;   // ohms/metre
  vector<ParasiticsCapacitance> wire_clk_cap_;  // Farads/meter
  LibertyCellSet dont_use_;
  double max_area_ = 0.0;

  Logger* logger_ = nullptr;
  SteinerTreeBuilder* stt_builder_ = nullptr;
  GlobalRouter* global_router_ = nullptr;
  IncrementalGRoute* incr_groute_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  dbDatabase* db_ = nullptr;
  dbBlock* block_ = nullptr;
  int dbu_ = 0;
  const Pin* debug_pin_ = nullptr;

  Rect core_;
  bool core_exists_ = false;

  ParasiticsSrc parasitics_src_ = ParasiticsSrc::none;
  UnorderedSet<const Net*, NetHash> parasitics_invalid_;

  double design_area_ = 0.0;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
  LibertyCellSeq buffer_cells_;
  LibertyCell* buffer_lowest_drive_ = nullptr;

  CellTargetLoadMap* target_load_map_ = nullptr;
  VertexSeq level_drvr_vertices_;
  bool level_drvr_vertices_valid_ = false;
  TgtSlews tgt_slews_;
  Corner* tgt_slew_corner_ = nullptr;
  const DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;
  // Instances with multiple output ports that have been resized.
  InstanceSet resized_multi_output_insts_;
  int unique_net_index_ = 1;
  int unique_inst_index_ = 1;
  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  int cloned_gate_count_ = 0;
  bool buffer_moved_into_core_ = false;
  // Slack map variables.
  // This is the minimum length of wire that is worth while to split and
  // insert a buffer in the middle of. Theoretically computed using the smallest
  // drive cell (because larger ones would give us a longer length).
  float max_wire_length_ = 0;
  float worst_slack_nets_percent_ = 10;
  Map<const Net*, Slack> net_slack_map_;
  NetSeq worst_slack_nets_;

  // Journal to roll back changes (OpenDB not up to the task).
  Map<Instance*, LibertyCell*> resized_inst_map_;
  InstanceSeq inserted_buffers_;
  InstanceSet inserted_buffer_set_;
  Map<Instance*, LibertyPortTuple> swapped_pins_;
  std::stack<InstanceTuple> cloned_gates_;
  std::unordered_set<Instance*> cloned_inst_set_;

  // Need to track all changes for buffer removal
  InstanceSet all_sized_inst_set_;
  InstanceSet all_inserted_buffer_set_;
  InstanceSet all_swapped_pin_inst_set_;
  InstanceSet all_cloned_inst_set_;

  dpl::Opendp* opendp_ = nullptr;

  // "factor debatable"
  static constexpr float tgt_slew_load_cap_factor = 10.0;
  // Prim/Dijkstra gets out of hand with bigger nets.
  static constexpr int max_steiner_pin_count_ = 200000;

  // Use actual input slews for accurate delay/slew estimation
  sta::UnorderedMap<LibertyPort*, InputSlews> input_slew_map_;

  friend class BufferedNet;
  friend class GateCloner;
  friend class PreChecks;
  friend class RecoverPower;
  friend class RepairDesign;
  friend class RepairSetup;
  friend class RepairHold;
  friend class SteinerTree;
};

}  // namespace rsz
