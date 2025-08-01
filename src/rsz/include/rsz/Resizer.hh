// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "grt/GlobalRouter.h"
#include "rsz/OdbCallBack.hh"
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

namespace sta {
class SpefWriter;
}

namespace rsz {

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
class Rebuffer;
class ResizerObserver;
class ConcreteSwapArithModules;

class CloneMove;
class BufferMove;
class SplitLoadMove;
class SizeDownMove;
class SizeUpMove;
class SwapPinsMove;
class UnbufferMove;
class RegisterOdbCallbackGuard;

class NetHash
{
 public:
  size_t operator()(const Net* net) const { return hashPtr(net); }
};

using CellTargetLoadMap = Map<LibertyCell*, float>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

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

enum class MoveType
{
  BUFFER,
  UNBUFFER,
  SWAP,
  SIZE,
  SIZEUP,
  SIZEDOWN,
  CLONE,
  SPLIT
};

// Voltage Threshold (VT) category identifier
struct VTCategory
{
  int vt_index;
  std::string vt_name;

  // Enable use as map key
  bool operator<(const VTCategory& other) const
  {
    if (vt_index != other.vt_index) {
      return vt_index < other.vt_index;
    }
    return vt_name < other.vt_name;
  }
};

// Leakage statistics for cells in a single VT category
struct VTLeakageStats
{
  int cell_count = 0;
  float total_leakage = 0.0f;

  float get_average_leakage() const
  {
    return cell_count > 0 ? total_leakage / cell_count : 0.0f;
  }

  void add_cell_leakage(std::optional<float> cell_leak)
  {
    cell_count++;
    if (cell_leak.has_value()) {
      total_leakage += *cell_leak;
    }
  }
};

// Complete analysis data for a library
struct LibraryAnalysisData
{
  // VT category leakage analysis
  std::map<VTCategory, VTLeakageStats> vt_leakage_by_category;
  // Cell footprint distribution (footprint_name -> count)
  std::map<std::string, int> cells_by_footprint;
  // LEF site usage distribution (site -> count)
  std::map<odb::dbSite*, int> cells_by_site;
  // VT categories sorted by VT type for HVT/RVT/LVT/uLVT ordering
  std::vector<std::pair<VTCategory, VTLeakageStats>> sorted_vt_categories;

  // Helper methods for common operations
  void sort_vt_categories()
  {
    sorted_vt_categories.clear();
    sorted_vt_categories.reserve(vt_leakage_by_category.size());
    for (const auto& vt_pair : vt_leakage_by_category) {
      sorted_vt_categories.push_back(vt_pair);
    }

    // Sort by average leakage (ascending order - least leaky to most leaky)
    std::sort(sorted_vt_categories.begin(),
              sorted_vt_categories.end(),
              [](const auto& a, const auto& b) {
                return a.second.get_average_leakage()
                       < b.second.get_average_leakage();
              });
  }
};

class OdbCallBack;

class Resizer : public dbStaState, public dbNetworkObserver
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

  // Core area (meters).
  double coreArea() const;
  // 0.0 - 1.0 (100%) of core size.
  double utilization();
  // Maximum utilizable area (core area * utilization)
  double maxArea() const;

  void setDontUse(LibertyCell* cell, bool dont_use);
  void resetDontUse();
  bool dontUse(const LibertyCell* cell);
  void reportDontUse() const;
  void setDontTouch(const Instance* inst, bool dont_touch);
  bool dontTouch(const Instance* inst);
  void setDontTouch(const Net* net, bool dont_touch);
  bool dontTouch(const Net* net);
  bool dontTouch(const Pin* pin);
  void reportDontTouch();

  void reportFastBufferSizes();

  void setMaxUtilization(double max_utilization);
  // Remove all or selected buffers from the netlist.
  void removeBuffers(InstanceSeq insts);
  void unbufferNet(Net* net);
  void bufferInputs(LibertyCell* buffer_cell = nullptr, bool verbose = false);
  void bufferOutputs(LibertyCell* buffer_cell = nullptr, bool verbose = false);

  // from sta::dbNetworkObserver callbacks
  void postReadLiberty() override;

  // Balance the usage of hybrid rows
  void balanceRowUsage();

  // Resize drvr_pin instance to target slew.
  void resizeDrvrToTargetSlew(const Pin* drvr_pin);
  // Accessor for debugging.
  Slew targetSlew(const RiseFall* rf);
  // Accessor for debugging.
  float targetLoadCap(LibertyCell* cell);

  ////////////////////////////////////////////////////////////////
  bool repairSetup(double setup_margin,
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_repairs_per_pass,
                   bool match_cell_footprint,
                   bool verbose,
                   const std::vector<MoveType>& sequence,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_size_down,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp);
  // For testing.
  void repairSetup(const Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const Pin* drvr_pin);

  ////////////////////////////////////////////////////////////////

  bool repairHold(double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  // Max buffer count as percent of design instance count.
                  float max_buffer_percent,
                  int max_passes,
                  bool match_cell_footprint,
                  bool verbose);
  void repairHold(const Pin* end_pin,
                  double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  float max_buffer_percent,
                  int max_passes);
  int holdBufferCount() const;

  ////////////////////////////////////////////////////////////////
  bool recoverPower(float recover_power_percent,
                    bool match_cell_footprint,
                    bool verbose);

  ////////////////////////////////////////////////////////////////
  void swapArithModules(int path_count,
                        const std::string& target,
                        float slack_margin);

  ////////////////////////////////////////////////////////////////

  // Area of the design in meter^2.
  double designArea();
  // Increment design_area
  void designAreaIncr(float delta);
  // Caller owns return value.
  NetSeq* findFloatingNets();
  PinSet* findFloatingPins();
  NetSeq* findOverdrivenNets(bool include_parallel_driven);
  void repairTieFanout(LibertyPort* tie_port,
                       double separation,  // meters
                       bool verbose);
  void bufferWireDelay(LibertyCell* buffer_cell,
                       double wire_length,  // meters
                       Delay& delay,
                       Slew& slew);
  void setDebugPin(const Pin* pin);
  void setWorstSlackNetsPercent(float);
  void annotateInputSlews(Instance* inst, const DcalcAnalysisPt* dcalc_ap);
  void resetInputSlews();

  ////////////////////////////////////////////////////////////////

  // Repair long wires, max fanout violations.
  void repairDesign(
      double max_wire_length,  // max_wire_length zero for none (meters)
      double slew_margin,      // 0.0-1.0
      double cap_margin,       // 0.0-1.0
      double buffer_gain,
      bool match_cell_footprint,
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
  void setClockBuffersList(const LibertyCellSeq& clk_buffers)
  {
    clk_buffers_ = clk_buffers;
  }
  // Clone inverters next to the registers they drive to remove them
  // from the clock network.
  // yosys is too stupid to use the inverted clock registers
  // and TritonCTS is too stupid to balance clock networks with inverters.
  void repairClkInverters();

  void reportLongWires(int count, int digits);
  // Find the max wire length before it is faster to split the wire
  // in half with a buffer (in meters).
  double findMaxWireLength(bool issue_error = true);
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
  void findResizeSlacks(bool run_journal_restore);
  // Return nets with worst slack.
  NetSeq resizeWorstSlackNets();
  // Return net slack, if any (indicated by the bool).
  std::optional<Slack> resizeNetSlack(const Net* net);
  std::optional<Slack> resizeNetSlack(const dbNet* db_net);

  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  PinSet findFaninFanouts(PinSet& end_pins);
  PinSet findFanins(PinSet& end_pins);

  ////////////////////////////////////////////////////////////////
  void highlightSteiner(const Pin* drvr);

  dbNetwork* getDbNetwork() { return db_network_; }
  ParasiticsSrc getParasiticsSrc() { return parasitics_src_; }
  void setParasiticsSrc(ParasiticsSrc src);
  dbBlock* getDbBlock() { return block_; };
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;
  void makeEquivCells();
  std::pair<int, std::string> cellVTType(dbMaster* master);

  ////////////////////////////////////////////////////////////////
  void initBlock();
  void journalBeginTest();
  void journalRestoreTest();
  Logger* logger() const { return logger_; }
  void eraseParasitics(const Net* net);
  void eliminateDeadLogic(bool clean_nets);
  std::optional<float> cellLeakage(LibertyCell* cell);
  // For debugging - calls getSwappableCells
  void reportEquivalentCells(LibertyCell* base_cell,
                             bool match_cell_footprint,
                             bool report_all_cells);
  void reportBuffers(bool filtered);
  void getBufferList(LibertyCellSeq& buffer_list,
                     LibraryAnalysisData& lib_data);
  void setDebugGraphics(std::shared_ptr<ResizerObserver> graphics);

  static MoveType parseMove(const std::string& s);
  static std::vector<MoveType> parseMoveSequence(const std::string& sequence);
  void fullyRebuffer(Pin* pin);

 protected:
  void init();
  double computeDesignArea();
  void initDesignArea();
  void ensureLevelDrvrVertices();
  Instance* bufferInput(const Pin* top_pin,
                        LibertyCell* buffer_cell,
                        bool verbose);
  void bufferOutput(const Pin* top_pin, LibertyCell* buffer_cell, bool verbose);
  bool hasTristateOrDontTouchDriver(const Net* net);
  bool isTristateDriver(const Pin* pin);
  void checkLibertyForAllCorners();
  void copyDontUseFromLiberty();
  bool bufferSizeOutmatched(LibertyCell* worse,
                            LibertyCell* better,
                            float max_drive_resist);
  void findBuffers();
  void findBuffersNoPruning();
  void findFastBuffers();
  LibertyCell* selectBufferCell(LibertyCell* buffer_cell = nullptr);
  bool isLinkCell(LibertyCell* cell) const;
  void findTargetLoads();
  void balanceBin(const std::vector<odb::dbInst*>& bin,
                  const std::set<odb::dbSite*>& base_sites);

  //==============================
  // APIs for gate cloning
  LibertyCell* halfDrivingPowerCell(Instance* inst);
  LibertyCell* halfDrivingPowerCell(LibertyCell* cell);
  LibertyCell* closestDriver(LibertyCell* cell,
                             const LibertyCellSeq& candidates,
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
  LibertyCellSeq getSwappableCells(LibertyCell* source_cell);

  bool getCin(const LibertyCell* cell, float& cin);
  // Resize drvr_pin instance to target slew.
  // Return 1 if resized.
  int resizeToTargetSlew(const Pin* drvr_pin);

  // Resize drvr_pin instance to target cap ratio.
  // Return 1 if resized.
  int resizeToCapRatio(const Pin* drvr_pin, bool upsize_only);

  ////////////////////////////////////////////////////////////////

  void findLongWires(VertexSeq& drvrs);
  int findMaxSteinerDist(Vertex* drvr, const Corner* corner);
  float driveResistance(const Pin* drvr_pin);
  float bufferDriveResistance(const LibertyCell* buffer) const;
  float cellDriveResistance(const LibertyCell* cell) const;

  // Max distance from driver to load (in dbu).
  int maxLoadManhattenDistance(Vertex* drvr);

  double findMaxWireLength1(bool issue_error = true);
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
                     std::unique_ptr<dbSta>& sta,
                     // Return values.
                     Delay& delay,
                     Slew& slew);
  void makeWireParasitic(Net* net,
                         Pin* drvr_pin,
                         Pin* load_pin,
                         double wire_length,  // meters
                         const Corner* corner,
                         Parasitics* parasitics);
  std::string makeUniqueNetName(Instance* parent = nullptr);
  Net* makeUniqueNet();
  std::string makeUniqueInstName(const char* base_name);
  std::string makeUniqueInstName(const char* base_name, bool underscore);
  bool overMaxArea();
  bool bufferBetweenPorts(Instance* buffer);
  bool hasPort(const Net* net);
  Point location(Instance* inst);
  double area(dbMaster* master);
  double area(Cell* cell);
  double splitWireDelayDiff(double wire_length,
                            LibertyCell* buffer_cell,
                            std::unique_ptr<dbSta>& sta);
  double maxSlewWireDiff(LibertyPort* drvr_port,
                         LibertyPort* load_port,
                         double wire_length,
                         double max_slew);
  void bufferWireDelay(LibertyCell* buffer_cell,
                       double wire_length,  // meters
                       std::unique_ptr<dbSta>& sta,
                       Delay& delay,
                       Slew& slew);
  void findCellInstances(LibertyCell* cell,
                         // Return value.
                         InstanceSeq& insts);
  void findLoads(Pin* drvr_pin, PinSeq& loads);
  bool isFuncOneZero(const Pin* drvr_pin);
  bool hasPins(Net* net);
  void getPins(Net* net, PinVector& pins) const;
  void getPins(Instance* inst, PinVector& pins) const;
  void SwapNetNames(odb::dbITerm* iterm_to, odb::dbITerm* iterm_from);
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
  void estimateWireParasiticSteiner(const Pin* drvr_pin,
                                    const Net* net,
                                    SpefWriter* spef_writer);
  float totalLoad(SteinerTree* tree) const;
  float subtreeLoad(SteinerTree* tree,
                    float cap_per_micron,
                    SteinerPt pt) const;
  void makePadParasitic(const Net* net, SpefWriter* spef_writer);
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
  Instance* makeInstance(LibertyCell* cell,
                         const char* name,
                         Instance* parent,
                         const Point& loc);
  void getBufferPins(Instance* buffer, Pin*& ip_pin, Pin*& op_pin);

  Instance* makeBuffer(LibertyCell* cell,
                       const char* name,
                       Instance* parent,
                       const Point& loc);
  void setLocation(dbInst* db_inst, const Point& pt);
  LibertyCell* findTargetCell(LibertyCell* cell,
                              float load_cap,
                              bool revisiting_inst);
  // Returns nullptr if net has less than 2 pins or any pin is not placed.
  SteinerTree* makeSteinerTree(Point drvr_location,
                               const std::vector<Point>& sink_locations);
  SteinerTree* makeSteinerTree(const Pin* drvr_pin);
  BufferedNetPtr makeBufferedNet(const Pin* drvr_pin, const Corner* corner);
  BufferedNetPtr makeBufferedNetSteiner(const Pin* drvr_pin,
                                        const Corner* corner);
  BufferedNetPtr makeBufferedNetSteinerOverBnets(
      Point root,
      const std::vector<BufferedNetPtr>& sinks,
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
  ////////////////////////////////////////////////////////////////
  // Jounalling support for checkpointing and backing out changes
  // during repair timing.
  void journalBegin();
  void journalEnd();
  void journalRestore();
  void journalMakeBuffer(Instance* buffer);

  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  VertexSet findFaninFanouts(VertexSet& ends);
  VertexSet findFaninRoots(VertexSet& ends);
  VertexSet findFanouts(VertexSet& reg_outs);
  bool isRegOutput(Vertex* vertex);
  bool isRegister(Vertex* vertex);
  ////////////////////////////////////////////////////////////////

  // Components
  std::unique_ptr<RecoverPower> recover_power_;
  std::unique_ptr<RepairDesign> repair_design_;
  std::unique_ptr<RepairSetup> repair_setup_;
  std::unique_ptr<RepairHold> repair_hold_;
  std::unique_ptr<ConcreteSwapArithModules> swap_arith_modules_;
  std::unique_ptr<AbstractSteinerRenderer> steiner_renderer_;
  std::unique_ptr<Rebuffer> rebuffer_;

  // Layer RC per wire length indexed by layer->getNumber(), corner->index
  std::vector<std::vector<double>> layer_res_;  // ohms/meter
  std::vector<std::vector<double>> layer_cap_;  // Farads/meter
  // Signal wire RC indexed by corner->index
  std::vector<ParasiticsResistance> wire_signal_res_;   // ohms/metre
  std::vector<ParasiticsCapacitance> wire_signal_cap_;  // Farads/meter
  // Clock wire RC.
  std::vector<ParasiticsResistance> wire_clk_res_;   // ohms/metre
  std::vector<ParasiticsCapacitance> wire_clk_cap_;  // Farads/meter
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
  bool incremental_parasitics_enabled_ = false;

  double design_area_ = 0.0;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
  LibertyCellSeq buffer_cells_;
  LibertyCell* buffer_lowest_drive_ = nullptr;
  std::set<LibertyCell*> buffer_fast_sizes_;
  // Buffer list created by CTS kept here so that we use the
  // exact same buffers when reparing clock nets.
  LibertyCellSeq clk_buffers_;

  // Cache results of getSwappableCells() as this is expensive for large PDKs.
  std::unordered_map<LibertyCell*, LibertyCellSeq> swappable_cells_cache_;

  std::unique_ptr<CellTargetLoadMap> target_load_map_;
  VertexSeq level_drvr_vertices_;
  bool level_drvr_vertices_valid_ = false;
  TgtSlews tgt_slews_;
  Corner* tgt_slew_corner_ = nullptr;
  const DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;
  // Instances with multiple output ports that have been resized.
  InstanceSet resized_multi_output_insts_;
  int unique_net_index_ = 1;
  int unique_inst_index_ = 1;
  int inserted_buffer_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  bool exclude_clock_buffers_ = true;
  bool buffer_moved_into_core_ = false;
  bool match_cell_footprint_ = false;
  // Slack map variables.
  // This is the minimum length of wire that is worth while to split and
  // insert a buffer in the middle of. Theoretically computed using the smallest
  // drive cell (because larger ones would give us a longer length).
  float max_wire_length_ = 0;
  float worst_slack_nets_percent_ = 10;
  Map<const Net*, Slack> net_slack_map_;

  std::unordered_map<LibertyCell*, std::optional<float>> cell_leakage_cache_;

  InstanceSet inserted_buffer_set_;
  InstanceSet all_inserted_buffer_set_;
  InstanceSet removed_buffer_set_;

  dpl::Opendp* opendp_ = nullptr;

  // "factor debatable"
  static constexpr float tgt_slew_load_cap_factor = 10.0;

  // Use actual input slews for accurate delay/slew estimation
  sta::UnorderedMap<LibertyPort*, InputSlews> input_slew_map_;

  std::unique_ptr<OdbCallBack> db_cbk_;

  // Restrict default sizing such that one sizing move cannot increase area or
  // leakage by more than 4X.  Subsequent sizing moves can exceed the 4X limit.
  std::optional<double> sizing_area_limit_ = 4.0;
  std::optional<double> sizing_leakage_limit_ = 4.0;
  bool default_sizing_area_limit_set_ = true;
  bool default_sizing_leakage_limit_set_ = true;
  bool sizing_keep_site_ = false;
  bool sizing_keep_vt_ = false;
  bool disable_buffer_pruning_ = true;

  // Sizing
  const double default_sizing_cap_ratio_ = 4.0;
  const double default_buffer_sizing_cap_ratio_ = 9.0;
  double sizing_cap_ratio_;
  double buffer_sizing_cap_ratio_;

  // VT layer hash
  std::unordered_map<dbMaster*, std::pair<int, std::string>> vt_map_;
  std::unordered_map<size_t, int>
      vt_hash_map_;  // maps hash value to unique int

  std::shared_ptr<ResizerObserver> graphics_;

  // Optimization moves
  // Will eventually be replaced with a getter method and some "recipes"
  std::unique_ptr<CloneMove> clone_move_;
  std::unique_ptr<SplitLoadMove> split_load_move_;
  std::unique_ptr<BufferMove> buffer_move_;
  std::unique_ptr<SizeDownMove> size_down_move_;
  std::unique_ptr<SizeUpMove> size_up_move_;
  std::unique_ptr<SwapPinsMove> swap_pins_move_;
  std::unique_ptr<UnbufferMove> unbuffer_move_;
  int accepted_move_count_ = 0;
  int rejected_move_count_ = 0;

  friend class BufferedNet;
  friend class GateCloner;
  friend class PreChecks;
  friend class RecoverPower;
  friend class RepairDesign;
  friend class RepairSetup;
  friend class RepairHold;
  friend class SteinerTree;
  friend class BaseMove;
  friend class BufferMove;
  friend class SizeDownMove;
  friend class SizeUpMove;
  friend class SplitLoadMove;
  friend class CloneMove;
  friend class SwapPinsMove;
  friend class UnbufferMove;
  friend class SwapArithModules;
  friend class ConcreteSwapArithModules;
  friend class IncrementalParasiticsGuard;
  friend class Rebuffer;
  friend class OdbCallBack;
};

class IncrementalParasiticsGuard
{
 public:
  IncrementalParasiticsGuard(Resizer* resizer);
  ~IncrementalParasiticsGuard();

  // calls resizer_->updateParasitics()
  void update();

 private:
  Resizer* resizer_;
  bool need_unregister_;
};

}  // namespace rsz
