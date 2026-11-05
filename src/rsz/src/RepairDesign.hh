// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "BufferedNet.hh"
#include "PreChecks.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;
enum class ParasiticsSrc;

// Region for partioning fanout pins.
class LoadRegion
{
 public:
  LoadRegion();
  LoadRegion(sta::PinSeq& pins, odb::Rect& bbox);

  sta::PinSeq pins_;
  odb::Rect bbox_;  // dbu
  std::vector<LoadRegion> regions_;
};

struct RepeaterSizingHint
{
  std::optional<float> load_cap;
  float max_slew = sta::INF;
};

class RepairDesign : sta::dbStaState
{
 public:
  explicit RepairDesign(Resizer* resizer);
  ~RepairDesign() override;
  void repairDesign(double max_wire_length,
                    double slew_margin,
                    double cap_margin,
                    double buffer_gain,
                    bool verbose);
  void repairDesign(double max_wire_length,  // zero for none (meters)
                    double slew_margin,
                    double cap_margin,
                    bool initial_sizing,
                    bool verbose,
                    int& repaired_net_count,
                    int& slew_violations,
                    int& cap_violations,
                    int& fanout_violations,
                    int& length_violations);
  bool rerouteEnabled() const;
  int insertedBufferCount() const { return inserted_buffer_count_; }
  void repairNet(sta::Net* net,
                 double max_wire_length,
                 double slew_margin,
                 double cap_margin);
  void repairClkNets(double max_wire_length);
  void repairClkInverters();
  void reportViolationCounters(bool invalidate_driver_vertices,
                               int slew_violations,
                               int cap_violations,
                               int fanout_violations,
                               int length_violations,
                               int repaired_net_count);
  void setDebugGraphics(std::shared_ptr<ResizerObserver> graphics);

 protected:
  void init();

  bool getCin(const sta::Pin* drvr_pin, float& cin);
  bool getLargestSizeCin(const sta::Pin* drvr_pin, float& cin);
  void findBufferSizes();
  bool performGainBuffering(sta::Net* net,
                            const sta::Pin* drvr_pin,
                            int max_fanout);
  void performEarlySizingRound(int& repaired_net_count);

  void checkDriverArcSlew(const sta::Scene* corner,
                          const sta::Instance* inst,
                          const sta::TimingArc* arc,
                          float load_cap,
                          float limit,
                          float& violation);
  bool repairDriverSlew(const sta::Scene* corner, const sta::Pin* drvr_pin);

  void repairDriver(sta::Vertex* drvr,
                    bool check_slew,
                    bool check_cap,
                    bool check_fanout,
                    int max_length,  // dbu
                    bool resize_drvr,
                    sta::Scene* corner_w_load_slew_viol,
                    int& repaired_net_count,
                    int& slew_violations,
                    int& cap_violations,
                    int& fanout_violations,
                    int& length_violations);

  void repairNet(sta::Net* net,
                 const sta::Pin* drvr_pin,
                 sta::Vertex* drvr,
                 bool check_slew,
                 bool check_cap,
                 bool check_fanout,
                 int max_length,  // dbu
                 bool resize_drvr,
                 sta::Scene* corner_w_load_slew_viol,  // if not null, signals
                                                       // a violation hidden by
                                                       // an annotation
                 int& repaired_net_count,
                 int& slew_violations,
                 int& cap_violations,
                 int& fanout_violations,
                 int& length_violations);
  bool needRepairCap(const sta::Pin* drvr_pin,
                     int& cap_violations,
                     float& max_cap,
                     const sta::Scene*& corner);
  bool needRepairWire(int max_length, int wire_length, int& length_violations);
  void checkSlew(const sta::Pin* drvr_pin,
                 // Return values.
                 sta::Slew& slew,
                 float& limit,
                 float& slack,
                 const sta::Scene*& corner);
  float bufferInputMaxSlew(sta::LibertyCell* buffer,
                           const sta::Scene* corner) const;
  void repairNet(const BufferedNetPtr& bnet,
                 const sta::Pin* drvr_pin,
                 float max_cap,
                 int max_length,  // dbu
                 const sta::Scene* corner);
  void repairNet(const BufferedNetPtr& bnet,
                 int level,
                 // Return values.
                 int& wire_length,
                 sta::PinSeq& load_pins);
  void checkSlewLimit(float ref_cap, float max_load_slew);
  void repairNetVia(const BufferedNetPtr& bnet,
                    int level,
                    // Return values.
                    int& wire_length,
                    sta::PinSeq& load_pins);
  void repairNetWire(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     sta::PinSeq& load_pins);
  void repairNetJunc(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     sta::PinSeq& load_pins);
  void repairNetLoad(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     sta::PinSeq& load_pins);
  float maxSlewMargined(float max_slew);
  double findSlewLoadCap(sta::LibertyPort* drvr_port,
                         double slew,
                         const sta::Scene* corner);
  double gateSlewDiff(sta::LibertyPort* drvr_port,
                      double load_cap,
                      double slew,
                      const sta::Scene* corner,
                      const sta::MinMax* min_max);
  LoadRegion findLoadRegions(const sta::Net* net,
                             const sta::Pin* drvr_pin,
                             int max_fanout);
  void subdivideRegion(LoadRegion& region, int max_fanout);
  void makeRegionRepeaters(LoadRegion& region,
                           int max_fanout,
                           int level,
                           const sta::Pin* drvr_pin,
                           bool check_slew,
                           bool check_cap,
                           int max_length,
                           bool resize_drvr);
  void makeFanoutRepeater(sta::PinSeq& repeater_loads,
                          sta::PinSeq& repeater_inputs,
                          const odb::Rect& bbox,
                          const odb::Point& loc,
                          bool check_slew,
                          bool check_cap,
                          int max_length,
                          bool resize_drvr);
  sta::PinSeq findLoads(const sta::Pin* drvr_pin);
  odb::Rect findBbox(sta::PinSeq& pins);
  odb::Point findClosedPinLoc(const sta::Pin* drvr_pin, sta::PinSeq& pins);
  bool isRepeater(const sta::Pin* load_pin);
  struct BufferDrcResult
  {
    bool drc_clean = false;
    bool cap_clean = false;
    float slew = sta::INF;
  };
  struct BufferFallback
  {
    sta::LibertyCell* cap_clean_buf = nullptr;
    float cap_clean_slew = sta::INF;
    sta::LibertyCell* min_slew_buf = nullptr;
    float min_slew = sta::INF;
  };
  bool makeRepeater(const char* reason,
                    const odb::Point& loc,
                    sta::LibertyCell* buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    sta::PinSeq& load_pins,
                    float& repeater_cap,
                    float& repeater_fanout,
                    float& repeater_max_slew,
                    const RepeaterSizingHint& sizing_hint = {});
  bool makeRepeater(const char* reason,
                    int x,
                    int y,
                    sta::LibertyCell* buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    sta::PinSeq& load_pins,
                    float& repeater_cap,
                    float& repeater_fanout,
                    float& repeater_max_slew,
                    sta::Net*& out_net,
                    sta::Pin*& repeater_in_pin,
                    sta::Pin*& repeater_out_pin,
                    const RepeaterSizingHint& sizing_hint = {});
  sta::LibertyCell* findBufferUnderSlew(float max_slew, float load_cap);
  // Selects a DRC-clean buffer from ranked candidates and updates fallbacks.
  sta::LibertyCell* selectBufferFromCandidates(
      const sta::LibertyCellSeq& candidates,
      float load_cap,
      float max_slew,
      const sta::Scene* corner,
      bool honor_target_slew,
      BufferFallback& fallback);
  // Logs the chosen repeater cell and selection reason under debug level 1.
  void logBufferChoice(sta::LibertyCell* buf,
                       const char* reason,
                       float slew) const;
  // Sorts buffers by ascending area and prunes equal/weaker-drive cells.
  sta::LibertyCellSeq rankBufferCandidates(sta::LibertyCellSeq cells,
                                           bool clock_only = false) const;
  // Returns cached ranked buffer candidates within resizer sizing limits.
  const sta::LibertyCellSeq& softCandidates(sta::LibertyCell* seed);
  // Returns cached ranked buffer candidates with sizing limits lifted for DRC.
  const sta::LibertyCellSeq& unconstrainedCandidates(sta::LibertyCell* unit);
  // Returns cached ranked clock-buffer candidates for clock-net repair.
  const sta::LibertyCellSeq& clockCandidates(sta::LibertyCell* unit);
  // Evaluates max_capacitance and output max_transition limits for buf.
  BufferDrcResult isBufferDrcClean(sta::LibertyCell* buf,
                                   float load_cap,
                                   float max_slew,
                                   const sta::Scene* corner);
  bool hasInputPort(const sta::Net* net);
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  void printProgress(int iteration,
                     bool force,
                     bool end,
                     int repaired_net_count,
                     int total_vertices) const;

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  std::unique_ptr<PreChecks> pre_checks_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;
  int dbu_ = 0;
  double initial_design_area_ = 0;
  est::ParasiticsSrc parasitics_src_ = est::ParasiticsSrc::kNone;

  // Gain buffering
  std::vector<sta::LibertyCell*> buffer_sizes_;

  // Implicit arguments to repairNet bnet recursion.
  const sta::Pin* drvr_pin_ = nullptr;
  float max_cap_ = 0;
  int max_length_ = 0;
  double slew_margin_ = 0;
  double cap_margin_ = 0;
  const sta::Scene* corner_ = nullptr;

  bool reroute_ = false;
  int rerouted_nets_ = 0;

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  int print_interval_ = 0;
  std::shared_ptr<ResizerObserver> graphics_;

  float r_strongest_buffer_ = 0;
  struct BufferCandidates
  {
    sta::LibertyCell* seed = nullptr;
    sta::LibertyCellSeq soft;  // within resizer sizing limits
    std::optional<sta::LibertyCellSeq>
        unconstrained;  // sizing limits lifted, built lazily
    std::optional<sta::LibertyCellSeq>
        clock;  // clock buffers only, built lazily during repair_clock_nets
  };
  BufferCandidates buffer_candidates_;

  static constexpr int min_print_interval_ = 10;
  static constexpr int max_print_interval_ = 1000;

  friend class Resizer;
};

}  // namespace rsz
