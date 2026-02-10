// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "db_sta/SpefWriter.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/OdbCallBack.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Hash.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Map.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Parasitics.hh"
#include "sta/ParasiticsClass.hh"
#include "sta/Path.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/Transition.hh"
#include "sta/UnorderedMap.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace rsz {

// Buffer use classification
enum class BufferUse
{
  DATA,
  CLOCK
};

using LibertyPortTuple = std::tuple<sta::LibertyPort*, sta::LibertyPort*>;
using InstanceTuple = std::tuple<sta::Instance*, sta::Instance*>;
using InputSlews = std::array<sta::Slew, sta::RiseFall::index_count>;

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
class VTSwapSpeedMove;
class SizeUpMatchMove;
class RegisterOdbCallbackGuard;

class NetHash
{
 public:
  size_t operator()(const sta::Net* net) const { return hashPtr(net); }
};

using CellTargetLoadMap = sta::Map<sta::LibertyCell*, float>;
using TgtSlews = std::array<sta::Slew, sta::RiseFall::index_count>;

enum class MoveType
{
  BUFFER,
  UNBUFFER,
  SWAP,
  SIZE,
  SIZEUP,
  SIZEDOWN,
  CLONE,
  SPLIT,
  VTSWAP_SPEED,  // VT swap for timing (need VT swap for power also)
  SIZEUP_MATCH   // sizeup to match drive strength vs. prev stage
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
  bool operator==(const VTCategory& other) const
  {
    return (vt_index == other.vt_index && vt_name == other.vt_name);
  }
  bool operator!=(const VTCategory& other) const { return !(*this == other); }
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
      sorted_vt_categories.emplace_back(vt_pair);
    }

    // Sort by average leakage (ascending order - least leaky to most leaky)
    std::ranges::sort(sorted_vt_categories, [](const auto& a, const auto& b) {
      return a.second.get_average_leakage() < b.second.get_average_leakage();
    });
  }
};

class OdbCallBack;

class Resizer : public sta::dbStaState, public sta::dbNetworkObserver
{
 public:
  Resizer(utl::Logger* logger,
          odb::dbDatabase* db,
          sta::dbSta* sta,
          stt::SteinerTreeBuilder* stt_builder,
          grt::GlobalRouter* global_router,
          dpl::Opendp* opendp,
          est::EstimateParasitics* estimate_parasitics);
  ~Resizer() override;

  // Core area (meters).
  double coreArea() const;
  // 0.0 - 1.0 (100%) of core size.
  double utilization();
  // Maximum utilizable area (core area * utilization)
  double maxArea() const;

  sta::VertexSeq orderedLoadPinVertices();

  void setDontUse(sta::LibertyCell* cell, bool dont_use);
  void resetDontUse();
  bool dontUse(const sta::LibertyCell* cell);
  void reportDontUse() const;
  void setDontTouch(const sta::Instance* inst, bool dont_touch);
  bool dontTouch(const sta::Instance* inst) const;
  void setDontTouch(const sta::Net* net, bool dont_touch);
  bool dontTouch(const sta::Net* net) const;

  ///
  /// Wrapper for odb::dbNet::insertBufferAfterDriver().
  /// - This accepts STA objects instead of db objects.
  ///
  sta::Instance* insertBufferAfterDriver(sta::Net* net,
                                         sta::LibertyCell* buffer_cell,
                                         const odb::Point* loc = nullptr,
                                         const char* new_buf_base_name
                                         = kDefaultBufBaseName,
                                         const char* new_net_base_name
                                         = kDefaultNetBaseName,
                                         const odb::dbNameUniquifyType& uniquify
                                         = odb::dbNameUniquifyType::ALWAYS);
  odb::dbInst* insertBufferAfterDriver(odb::dbNet* net,
                                       odb::dbMaster* buffer_cell,
                                       const odb::Point* loc = nullptr,
                                       const char* new_buf_base_name
                                       = kDefaultBufBaseName,
                                       const char* new_net_base_name
                                       = kDefaultNetBaseName,
                                       const odb::dbNameUniquifyType& uniquify
                                       = odb::dbNameUniquifyType::ALWAYS);

  ///
  /// Wrapper for odb::dbNet::insertBufferBeforeLoad().
  /// - This accepts STA objects instead of db objects.
  ///
  sta::Instance* insertBufferBeforeLoad(sta::Pin* load_pin,
                                        sta::LibertyCell* buffer_cell,
                                        const odb::Point* loc = nullptr,
                                        const char* new_buf_base_name
                                        = kDefaultBufBaseName,
                                        const char* new_net_base_name
                                        = kDefaultNetBaseName,
                                        const odb::dbNameUniquifyType& uniquify
                                        = odb::dbNameUniquifyType::ALWAYS);
  odb::dbInst* insertBufferBeforeLoad(odb::dbObject* load_pin,
                                      odb::dbMaster* buffer_cell,
                                      const odb::Point* loc = nullptr,
                                      const char* new_buf_base_name
                                      = kDefaultBufBaseName,
                                      const char* new_net_base_name
                                      = kDefaultNetBaseName,
                                      const odb::dbNameUniquifyType& uniquify
                                      = odb::dbNameUniquifyType::ALWAYS);

  ///
  /// Wrapper for odb::dbNet::insertBufferBeforeLoads().
  /// - This accepts STA objects instead of db objects.
  ///
  sta::Instance* insertBufferBeforeLoads(
      sta::Net* net,
      sta::PinSeq* loads,
      sta::LibertyCell* buffer_cell,
      const odb::Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);
  sta::Instance* insertBufferBeforeLoads(
      sta::Net* net,
      sta::PinSet* loads,
      sta::LibertyCell* buffer_cell,
      const odb::Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);
  odb::dbInst* insertBufferBeforeLoads(
      odb::dbNet* net,
      const std::set<odb::dbObject*>& loads,
      odb::dbMaster* buffer_cell,
      const odb::Point* loc = nullptr,
      const char* new_buf_base_name = kDefaultBufBaseName,
      const char* new_net_base_name = kDefaultNetBaseName,
      const odb::dbNameUniquifyType& uniquify = odb::dbNameUniquifyType::ALWAYS,
      bool loads_on_diff_nets = false);
  bool dontTouch(const sta::Pin* pin) const;
  void reportDontTouch();

  void reportFastBufferSizes();

  void setMaxUtilization(double max_utilization);
  // Remove all or selected buffers from the netlist.
  void removeBuffers(sta::InstanceSeq insts);
  void unbufferNet(sta::Net* net);
  void bufferInputs(sta::LibertyCell* buffer_cell = nullptr,
                    bool verbose = false);
  void bufferOutputs(sta::LibertyCell* buffer_cell = nullptr,
                     bool verbose = false);

  // from sta::dbNetworkObserver callbacks
  void postReadLiberty() override;

  // Balance the usage of hybrid rows
  void balanceRowUsage();

  // Resize drvr_pin instance to target slew.
  void resizeDrvrToTargetSlew(const sta::Pin* drvr_pin);
  // Accessor for debugging.
  sta::Slew targetSlew(const sta::RiseFall* rf);
  // Accessor for debugging.
  float targetLoadCap(sta::LibertyCell* cell);

  ////////////////////////////////////////////////////////////////
  bool repairSetup(double setup_margin,
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_iterations,
                   int max_repairs_per_pass,
                   bool match_cell_footprint,
                   bool verbose,
                   const std::vector<MoveType>& sequence,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_size_down,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp,
                   bool skip_vt_swap,
                   bool skip_crit_vt_swap);
  // For testing.
  void repairSetup(const sta::Pin* end_pin);
  // For testing.
  void reportSwappablePins();
  // Rebuffer one net (for testing).
  // resizerPreamble() required.
  void rebufferNet(const sta::Pin* drvr_pin);

  ////////////////////////////////////////////////////////////////

  bool repairHold(double setup_margin,
                  double hold_margin,
                  bool allow_setup_violations,
                  // Max buffer count as percent of design instance count.
                  float max_buffer_percent,
                  int max_passes,
                  int max_iterations,
                  bool match_cell_footprint,
                  bool verbose);
  void repairHold(const sta::Pin* end_pin,
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
  sta::NetSeq* findFloatingNets();
  sta::PinSet* findFloatingPins();
  sta::NetSeq* findOverdrivenNets(bool include_parallel_driven);
  void repairTieFanout(sta::LibertyPort* tie_port,
                       double separation,  // meters
                       bool verbose);
  void bufferWireDelay(sta::LibertyCell* buffer_cell,
                       double wire_length,  // meters
                       sta::Delay& delay,
                       sta::Slew& slew);
  void setDebugPin(const sta::Pin* pin);
  void setWorstSlackNetsPercent(float);
  void annotateInputSlews(sta::Instance* inst,
                          const sta::DcalcAnalysisPt* dcalc_ap);
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
  void repairNet(sta::Net* net,
                 double max_wire_length,  // meters
                 double slew_margin,
                 double cap_margin);

  // Repair long wires from clock input pins to clock tree root buffer
  // because CTS ignores the issue.
  // no max_fanout/max_cap checks.
  // Use max_wire_length zero for none (meters)
  void repairClkNets(
      double max_wire_length);  // max_wire_length zero for none (meters)
  void setClockBuffersList(const sta::LibertyCellSeq& clk_buffers)
  {
    clk_buffers_ = clk_buffers;
  }
  void inferClockBufferList(const char* lib_name,
                            std::vector<std::string>& buffers);
  bool isClockCellCandidate(sta::LibertyCell* cell);

  // Clock buffer pattern configuration
  void setClockBufferString(const std::string& clk_str);
  void setClockBufferFootprint(const std::string& footprint);
  void resetClockBufferPattern();
  bool hasClockBufferString() const { return !clock_buffer_string_.empty(); }
  bool hasClockBufferFootprint() const
  {
    return !clock_buffer_footprint_.empty();
  }
  const std::string& getClockBufferString() const
  {
    return clock_buffer_string_;
  }
  const std::string& getClockBufferFootprint() const
  {
    return clock_buffer_footprint_;
  }
  BufferUse getBufferUse(sta::LibertyCell* buffer);

  // Clone inverters next to the registers they drive to remove them
  // from the clock network.
  // yosys is too stupid to use the inverted clock registers
  // and TritonCTS is too stupid to balance clock networks with inverters.
  void repairClkInverters();

  void reportLongWires(int count, int digits);
  // Find the max wire length before it is faster to split the wire
  // in half with a buffer (in meters).
  double findMaxWireLength(bool issue_error = true);
  double findMaxWireLength(sta::LibertyCell* buffer_cell,
                           const sta::Corner* corner);
  double findMaxWireLength(sta::LibertyPort* drvr_port,
                           const sta::Corner* corner);
  // Longest driver to load wire (in meters).
  double maxLoadManhattenDistance(const sta::Net* net);

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
  sta::NetSeq resizeWorstSlackNets();
  // Return net slack, if any (indicated by the bool).
  std::optional<sta::Slack> resizeNetSlack(const sta::Net* net);
  std::optional<sta::Slack> resizeNetSlack(const odb::dbNet* db_net);

  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  sta::PinSet findFaninFanouts(sta::PinSet& end_pins);
  sta::PinSet findFanins(sta::PinSet& end_pins);

  ////////////////////////////////////////////////////////////////
  sta::dbNetwork* getDbNetwork() { return db_network_; }
  odb::dbBlock* getDbBlock() { return block_; }
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;
  void makeEquivCells();
  VTCategory cellVTType(odb::dbMaster* master);

  ////////////////////////////////////////////////////////////////
  void initBlock();
  void journalBeginTest();
  void journalRestoreTest();
  utl::Logger* logger() const { return logger_; }
  void eliminateDeadLogic(bool clean_nets);
  std::optional<float> cellLeakage(sta::LibertyCell* cell);
  // For debugging - calls getSwappableCells
  void reportEquivalentCells(sta::LibertyCell* base_cell,
                             bool match_cell_footprint,
                             bool report_all_cells,
                             bool report_vt_equiv);
  void reportBuffers(bool filtered);
  void getBufferList(sta::LibertyCellSeq& buffer_list);
  void setDebugGraphics(std::shared_ptr<ResizerObserver> graphics);

  static MoveType parseMove(const std::string& s);
  static std::vector<MoveType> parseMoveSequence(const std::string& sequence);
  void fullyRebuffer(sta::Pin* pin);

  bool hasFanout(sta::Vertex* drvr);
  bool hasFanout(sta::Pin* drvr);

  est::EstimateParasitics* getEstimateParasitics()
  {
    return estimate_parasitics_;
  }

  // Library analysis data
  std::unique_ptr<LibraryAnalysisData> lib_data_;

  // Compute slew RC factor based on library slew thresholds
  float getSlewRCFactor() const;

  sta::Slew findDriverSlewForLoad(sta::Pin* drvr_pin,
                                  float load,
                                  const sta::Corner* corner);
  bool computeNewDelaysSlews(
      sta::Pin* driver_pin,
      sta::Instance* buffer,
      const sta::Corner* corner,
      // return values
      sta::ArcDelay old_delay[sta::RiseFall::index_count],
      sta::ArcDelay new_delay[sta::RiseFall::index_count],
      sta::Slew old_drvr_slew[sta::RiseFall::index_count],
      sta::Slew new_drvr_slew[sta::RiseFall::index_count],
      // caps seen by driver_pin
      float& old_load_cap,
      float& new_load_cap);
  bool estimateSlewsAfterBufferRemoval(
      sta::Pin* drvr_pin,
      sta::Instance* buffer_instance,
      sta::Slew drvr_slew,
      const sta::Corner* corner,
      std::map<const sta::Pin*, float>& load_pin_slew);
  bool estimateSlewsInTree(sta::Pin* drvr_pin,
                           sta::Slew drvr_slew,
                           const BufferedNetPtr& tree,
                           const sta::Corner* corner,
                           std::map<const sta::Pin*, float>& load_pin_slew);

 protected:
  void init();
  double computeDesignArea();
  void initDesignArea();
  void ensureLevelDrvrVertices();
  sta::Instance* bufferInput(const sta::Pin* top_pin,
                             sta::LibertyCell* buffer_cell,
                             bool verbose);
  void bufferOutput(const sta::Pin* top_pin,
                    sta::LibertyCell* buffer_cell,
                    bool verbose);
  bool hasTristateOrDontTouchDriver(const sta::Net* net);
  bool isTristateDriver(const sta::Pin* pin) const;
  void checkLibertyForAllCorners();
  void copyDontUseFromLiberty();
  bool bufferSizeOutmatched(sta::LibertyCell* worse,
                            sta::LibertyCell* better,
                            float max_drive_resist);
  void findBuffers();
  void findBuffersNoPruning();
  void findFastBuffers();
  sta::LibertyCell* selectBufferCell(sta::LibertyCell* buffer_cell = nullptr);
  bool isLinkCell(sta::LibertyCell* cell) const;
  void findTargetLoads();
  void balanceBin(const std::vector<odb::dbInst*>& bin,
                  const std::set<odb::dbSite*>& base_sites);

  //==============================
  // APIs for gate cloning
  sta::LibertyCell* halfDrivingPowerCell(sta::Instance* inst);
  sta::LibertyCell* halfDrivingPowerCell(sta::LibertyCell* cell);
  sta::LibertyCell* closestDriver(sta::LibertyCell* cell,
                                  const sta::LibertyCellSeq& candidates,
                                  float scale);
  std::vector<sta::LibertyPort*> libraryPins(sta::Instance* inst) const;
  std::vector<sta::LibertyPort*> libraryPins(sta::LibertyCell* cell) const;
  bool isSingleOutputCombinational(sta::Instance* inst) const;
  bool isSingleOutputCombinational(sta::LibertyCell* cell) const;
  bool isCombinational(sta::LibertyCell* cell) const;
  std::vector<sta::LibertyPort*> libraryOutputPins(
      sta::LibertyCell* cell) const;
  float maxLoad(sta::Cell* cell);
  //==============================
  float findTargetLoad(sta::LibertyCell* cell);
  float findTargetLoad(sta::LibertyCell* cell,
                       sta::TimingArc* arc,
                       sta::Slew in_slew,
                       sta::Slew out_slew);
  sta::Slew gateSlewDiff(sta::LibertyCell* cell,
                         sta::TimingArc* arc,
                         sta::GateTimingModel* model,
                         sta::Slew in_slew,
                         float load_cap,
                         sta::Slew out_slew);
  void findBufferTargetSlews();
  void findBufferTargetSlews(sta::LibertyCell* buffer,
                             const sta::Pvt* pvt,
                             // Return values.
                             sta::Slew slews[],
                             int counts[]);
  bool hasMultipleOutputs(const sta::Instance* inst);

  void resizePreamble();
  sta::LibertyCellSeq getSwappableCells(sta::LibertyCell* source_cell);
  sta::LibertyCellSeq getVTEquivCells(sta::LibertyCell* source_cell);

  bool getCin(const sta::LibertyCell* cell, float& cin);
  // Resize drvr_pin instance to target slew.
  // Return 1 if resized.
  int resizeToTargetSlew(const sta::Pin* drvr_pin);

  // Resize drvr_pin instance to target cap ratio.
  // Return 1 if resized.
  int resizeToCapRatio(const sta::Pin* drvr_pin, bool upsize_only);

  ////////////////////////////////////////////////////////////////

  void findLongWires(sta::VertexSeq& drvrs);
  int findMaxSteinerDist(sta::Vertex* drvr, const sta::Corner* corner);
  float driveResistance(const sta::Pin* drvr_pin);
  float bufferDriveResistance(const sta::LibertyCell* buffer) const;
  float cellDriveResistance(const sta::LibertyCell* cell) const;

  // Max distance from driver to load (in dbu).
  int maxLoadManhattenDistance(sta::Vertex* drvr);

  double findMaxWireLength1(bool issue_error = true);
  float portFanoutLoad(sta::LibertyPort* port) const;
  float portCapacitance(sta::LibertyPort* input,
                        const sta::Corner* corner) const;
  float pinCapacitance(const sta::Pin* pin,
                       const sta::DcalcAnalysisPt* dcalc_ap) const;
  void swapPins(sta::Instance* inst,
                sta::LibertyPort* port1,
                sta::LibertyPort* port2,
                bool journal);
  void findSwapPinCandidate(sta::LibertyPort* input_port,
                            sta::LibertyPort* drvr_port,
                            const sta::LibertyPortSet& equiv_ports,
                            float load_cap,
                            const sta::DcalcAnalysisPt* dcalc_ap,
                            // Return value
                            sta::LibertyPort** swap_port);
  void gateDelays(const sta::LibertyPort* drvr_port,
                  float load_cap,
                  const sta::DcalcAnalysisPt* dcalc_ap,
                  // Return values.
                  sta::ArcDelay delays[sta::RiseFall::index_count],
                  sta::Slew slews[sta::RiseFall::index_count]);
  void gateDelays(const sta::LibertyPort* drvr_port,
                  float load_cap,
                  const sta::Slew in_slews[sta::RiseFall::index_count],
                  const sta::DcalcAnalysisPt* dcalc_ap,
                  // Return values.
                  sta::ArcDelay delays[sta::RiseFall::index_count],
                  sta::Slew out_slews[sta::RiseFall::index_count]);
  sta::ArcDelay gateDelay(const sta::LibertyPort* drvr_port,
                          float load_cap,
                          const sta::DcalcAnalysisPt* dcalc_ap);
  sta::ArcDelay gateDelay(const sta::LibertyPort* drvr_port,
                          const sta::RiseFall* rf,
                          float load_cap,
                          const sta::DcalcAnalysisPt* dcalc_ap);
  float bufferDelay(sta::LibertyCell* buffer_cell,
                    float load_cap,
                    const sta::DcalcAnalysisPt* dcalc_ap);
  float bufferDelay(sta::LibertyCell* buffer_cell,
                    const sta::RiseFall* rf,
                    float load_cap,
                    const sta::DcalcAnalysisPt* dcalc_ap);
  void bufferDelays(sta::LibertyCell* buffer_cell,
                    float load_cap,
                    const sta::DcalcAnalysisPt* dcalc_ap,
                    // Return values.
                    sta::ArcDelay delays[sta::RiseFall::index_count],
                    sta::Slew slews[sta::RiseFall::index_count]);
  void cellWireDelay(sta::LibertyPort* drvr_port,
                     sta::LibertyPort* load_port,
                     double wire_length,  // meters
                     std::unique_ptr<sta::dbSta>& sta,
                     // Return values.
                     sta::Delay& delay,
                     sta::Slew& slew);
  void makeWireParasitic(sta::Net* net,
                         sta::Pin* drvr_pin,
                         sta::Pin* load_pin,
                         double wire_length,  // meters
                         const sta::Corner* corner,
                         sta::Parasitics* parasitics);
  bool overMaxArea();
  bool bufferBetweenPorts(sta::Instance* buffer);
  bool hasPort(const sta::Net* net);
  odb::Point location(sta::Instance* inst);
  double area(odb::dbMaster* master);
  double area(sta::Cell* cell);
  double splitWireDelayDiff(double wire_length,
                            sta::LibertyCell* buffer_cell,
                            std::unique_ptr<sta::dbSta>& sta);
  double maxSlewWireDiff(sta::LibertyPort* drvr_port,
                         sta::LibertyPort* load_port,
                         double wire_length,
                         double max_slew);
  void bufferWireDelay(sta::LibertyCell* buffer_cell,
                       double wire_length,  // meters
                       std::unique_ptr<sta::dbSta>& sta,
                       sta::Delay& delay,
                       sta::Slew& slew);
  void findCellInstances(sta::LibertyCell* cell,
                         // Return value.
                         sta::InstanceSeq& insts);
  void findLoads(sta::Pin* drvr_pin, sta::PinSeq& loads);
  bool isFuncOneZero(const sta::Pin* drvr_pin);
  bool hasPins(sta::Net* net);
  void getPins(sta::Net* net, PinVector& pins) const;
  void getPins(sta::Instance* inst, PinVector& pins) const;
  odb::Point tieLocation(const sta::Pin* load, int separation);
  sta::InstanceSeq findClkInverters();
  void cloneClkInverter(sta::Instance* inv);

  void makePadParasitic(const sta::Net* net, sta::SpefWriter* spef_writer);
  bool isPadNet(const sta::Net* net) const;
  bool isPadPin(const sta::Pin* pin) const;
  bool isPad(const sta::Instance* inst) const;
  void net2Pins(const sta::Net* net,
                const sta::Pin*& pin1,
                const sta::Pin*& pin2) const;
  void parasiticNodeConnectPins(sta::Parasitic* parasitic,
                                sta::ParasiticNode* node,
                                est::SteinerTree* tree,
                                SteinerPt pt,
                                size_t& resistor_id);

  bool replaceCell(sta::Instance* inst,
                   const sta::LibertyCell* replacement,
                   bool journal);

  void findResizeSlacks1();
  sta::Instance* makeInstance(sta::LibertyCell* cell,
                              const char* name,
                              sta::Instance* parent,
                              const odb::Point& loc,
                              const odb::dbNameUniquifyType& uniquify
                              = odb::dbNameUniquifyType::ALWAYS);
  void deleteTieCellAndNet(const sta::Instance* tie_inst,
                           sta::LibertyPort* tie_port);
  const sta::Pin* findArithBoundaryPin(const sta::Pin* load_pin);
  sta::Instance* createNewTieCellForLoadPin(const sta::Pin* load_pin,
                                            const char* new_inst_name,
                                            sta::Instance* parent,
                                            sta::LibertyPort* tie_port,
                                            int separation_dbu);
  void getBufferPins(sta::Instance* buffer,
                     sta::Pin*& ip_pin,
                     sta::Pin*& op_pin);

  sta::Instance* makeBuffer(sta::LibertyCell* cell,
                            const char* name,
                            sta::Instance* parent,
                            const odb::Point& loc);

  void insertBufferPostProcess(odb::dbInst* buffer_inst);

  void setLocation(odb::dbInst* db_inst, const odb::Point& pt);
  sta::LibertyCell* findTargetCell(sta::LibertyCell* cell,
                                   float load_cap,
                                   bool revisiting_inst);
  BufferedNetPtr makeBufferedNet(const sta::Pin* drvr_pin,
                                 const sta::Corner* corner);
  BufferedNetPtr makeBufferedNetSteiner(const sta::Pin* drvr_pin,
                                        const sta::Corner* corner);
  BufferedNetPtr makeBufferedNetSteinerOverBnets(
      odb::Point root,
      const std::vector<BufferedNetPtr>& sinks,
      const sta::Corner* corner);
  BufferedNetPtr makeBufferedNetGroute(const sta::Pin* drvr_pin,
                                       const sta::Corner* corner);
  float bufferSlew(sta::LibertyCell* buffer_cell,
                   float load_cap,
                   const sta::DcalcAnalysisPt* dcalc_ap);
  float maxInputSlew(const sta::LibertyPort* input,
                     const sta::Corner* corner) const;
  void checkLoadSlews(const sta::Pin* drvr_pin,
                      double slew_margin,
                      // Return values.
                      sta::Slew& slew,
                      float& limit,
                      float& slack,
                      const sta::Corner*& corner);
  void warnBufferMovedIntoCore();
  bool isLogicStdCell(const sta::Instance* inst);

  bool okToBufferNet(const sta::Pin* driver_pin) const;
  bool checkAndMarkVTSwappable(sta::Instance* inst,
                               std::unordered_set<sta::Instance*>& notSwappable,
                               sta::LibertyCell*& best_lib_cell);

  BufferedNetPtr stitchTrees(const BufferedNetPtr& outer_tree,
                             sta::Pin* stitching_load,
                             const BufferedNetPtr& inner_tree);

  ////////////////////////////////////////////////////////////////
  // Jounalling support for checkpointing and backing out changes
  // during repair timing.
  void journalBegin();
  void journalEnd();
  void journalRestore();
  void journalMakeBuffer(sta::Instance* buffer);

  ////////////////////////////////////////////////////////////////
  // API for logic resynthesis
  sta::VertexSet findFaninFanouts(sta::VertexSet& ends);
  sta::VertexSet findFaninRoots(sta::VertexSet& ends);
  sta::VertexSet findFanouts(sta::VertexSet& reg_outs);
  bool isRegOutput(sta::Vertex* vertex);
  bool isRegister(sta::Vertex* vertex);
  ////////////////////////////////////////////////////////////////

  // Components
  std::unique_ptr<RecoverPower> recover_power_;
  std::unique_ptr<RepairDesign> repair_design_;
  std::unique_ptr<RepairSetup> repair_setup_;
  std::unique_ptr<RepairHold> repair_hold_;
  std::unique_ptr<ConcreteSwapArithModules> swap_arith_modules_;
  std::unique_ptr<Rebuffer> rebuffer_;

  // Layer RC per wire length indexed by layer->getNumber(), corner->index
  sta::LibertyCellSet dont_use_;
  double max_area_ = 0.0;

  utl::Logger* logger_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  stt::SteinerTreeBuilder* stt_builder_ = nullptr;
  grt::GlobalRouter* global_router_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  int dbu_ = 0;
  const sta::Pin* debug_pin_ = nullptr;

  odb::Rect core_;
  bool core_exists_ = false;

  double design_area_ = 0.0;
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();
  sta::LibertyCellSeq buffer_cells_;
  sta::LibertyCell* buffer_lowest_drive_ = nullptr;
  std::unordered_set<sta::LibertyCell*> buffer_fast_sizes_;
  // Buffer list created by CTS kept here so that we use the
  // exact same buffers when reparing clock nets.
  sta::LibertyCellSeq clk_buffers_;

  // Cache results of getSwappableCells() as this is expensive for large PDKs.
  std::unordered_map<sta::LibertyCell*, sta::LibertyCellSeq>
      swappable_cells_cache_;
  // Cache VT equivalent cells for each cell, equivalent cells are sorted in
  // increasing order of leakage
  // BUF_X1_RVT : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
  // BUF_X1_LVT : { BUF_X1_RVT, BUF_X1_LVT, BUF_X1_SLVT }
  // ...
  std::unordered_map<sta::LibertyCell*, sta::LibertyCellSeq>
      vt_equiv_cells_cache_;

  std::unique_ptr<CellTargetLoadMap> target_load_map_;
  sta::VertexSeq level_drvr_vertices_;
  bool level_drvr_vertices_valid_ = false;
  TgtSlews tgt_slews_;
  sta::Corner* tgt_slew_corner_ = nullptr;
  const sta::DcalcAnalysisPt* tgt_slew_dcalc_ap_ = nullptr;
  // Instances with multiple output ports that have been resized.
  sta::InstanceSet resized_multi_output_insts_;
  int inserted_buffer_count_ = 0;
  int cloned_gate_count_ = 0;
  int swap_pin_count_ = 0;
  int removed_buffer_count_ = 0;
  bool exclude_clock_buffers_ = true;
  bool buffer_moved_into_core_ = false;
  bool match_cell_footprint_ = false;
  bool equiv_cells_made_ = false;

  // Slack map variables.
  // This is the minimum length of wire that is worth while to split and
  // insert a buffer in the middle of. Theoretically computed using the smallest
  // drive cell (because larger ones would give us a longer length).
  float max_wire_length_ = 0;
  float worst_slack_nets_percent_ = 10;
  sta::Map<const sta::Net*, sta::Slack> net_slack_map_;

  std::unordered_map<sta::LibertyCell*, std::optional<float>>
      cell_leakage_cache_;

  sta::InstanceSet inserted_buffer_set_;
  sta::InstanceSet all_inserted_buffer_set_;
  sta::InstanceSet removed_buffer_set_;

  dpl::Opendp* opendp_ = nullptr;

  // "factor debatable"
  static constexpr float tgt_slew_load_cap_factor = 10.0;

  // Use actual input slews for accurate delay/slew estimation
  sta::UnorderedMap<sta::LibertyPort*, InputSlews> input_slew_map_;

  std::unique_ptr<OdbCallBack> db_cbk_;

  // Restrict default sizing such that one sizing move cannot increase area or
  // leakage by more than 4X.  Subsequent sizing moves can exceed the 4X limit.
  std::optional<double> sizing_area_limit_ = 4.0;
  std::optional<double> sizing_leakage_limit_ = 4.0;
  bool default_sizing_area_limit_set_ = true;
  bool default_sizing_leakage_limit_set_ = true;
  bool sizing_keep_site_ = false;
  bool sizing_keep_vt_ = false;
  bool disable_buffer_pruning_ = false;

  // Clock buffer pattern configuration
  std::string clock_buffer_string_;
  std::string clock_buffer_footprint_;

  // Sizing
  const double default_sizing_cap_ratio_ = 4.0;
  const double default_buffer_sizing_cap_ratio_ = 9.0;
  double sizing_cap_ratio_{default_sizing_cap_ratio_};
  double buffer_sizing_cap_ratio_{default_buffer_sizing_cap_ratio_};

  // VT layer hash
  std::unordered_map<odb::dbMaster*, VTCategory> vt_map_;
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
  std::unique_ptr<VTSwapSpeedMove> vt_swap_speed_move_;
  std::unique_ptr<SizeUpMatchMove> size_up_match_move_;
  int accepted_move_count_ = 0;
  int rejected_move_count_ = 0;

  friend class BufferedNet;
  friend class GateCloner;
  friend class PreChecks;
  friend class RecoverPower;
  friend class RepairDesign;
  friend class RepairSetup;
  friend class RepairHold;
  friend class BaseMove;
  friend class BufferMove;
  friend class SizeDownMove;
  friend class SizeUpMove;
  friend class SplitLoadMove;
  friend class CloneMove;
  friend class SwapPinsMove;
  friend class UnbufferMove;
  friend class SizeUpMatchMove;
  friend class VTSwapSpeedMove;
  friend class SwapArithModules;
  friend class ConcreteSwapArithModules;
  friend class Rebuffer;
  friend class OdbCallBack;
};

}  // namespace rsz
