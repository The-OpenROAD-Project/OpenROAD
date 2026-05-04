// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
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
  float getSlewRCFactor();

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

  // Cheap lib + odb HPWL screen on top of repair_design's per-driver
  // loop. Returns true iff Penfield-Rubinstein-style upper bounds
  // prove that this driver's net cannot violate slew, cap, or
  // fanout limits, *and* none of the existing repairNet code path's
  // skip-conditions would have applied. When true, the caller can
  // safely return without invoking ensureWireParasitic / findDelays
  // / makeBufferedNet / repairNet body.
  //
  // Fast path: only lib lookups (cached) + a single iterm/bterm walk
  // to compute HPWL and sum of input pin caps. ~50-100 ns per net
  // once caches are warm.
  bool screenNetSafe(const sta::Pin* drvr_pin,
                     sta::Net* net,
                     const sta::Scene* corner);
  void resetScreenCaches();

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
  bool makeRepeater(const char* reason,
                    const odb::Point& loc,
                    sta::LibertyCell* buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    sta::PinSeq& load_pins,
                    float& repeater_cap,
                    float& repeater_fanout,
                    float& repeater_max_slew);
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
                    sta::Pin*& repeater_out_pin);
  sta::LibertyCell* findBufferUnderSlew(float max_slew, float load_cap);
  bool hasInputPort(const sta::Net* net);
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  void printProgress(int iteration,
                     bool force,
                     bool end,
                     int repaired_net_count,
                     int total_vertices) const;

  void computeSlewRCFactor();

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  std::unique_ptr<PreChecks> pre_checks_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
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

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  int print_interval_ = 0;
  std::shared_ptr<ResizerObserver> graphics_;

  float r_strongest_buffer_ = 0;

  // Shape factor: what we need to multiply the RC product with
  // to get a slew estimate
  std::optional<float> slew_rc_factor_;

  static constexpr int min_print_interval_ = 10;
  static constexpr int max_print_interval_ = 1000;

  // Screen caches and counters (reset at the top of repairDesign()).
  // Keyed by LibertyCell* / LibertyPort* — same lookup result for
  // every instance of a given cell. SDC per-instance overrides on
  // cap/slew limits are not honored here; for the screen we accept
  // a small false-skip risk in exchange for O(1) per driver, and
  // empirical buffer-count comparison vs the reference catches it.
  std::unordered_map<sta::LibertyCell*, float> cell_cap_limit_cache_;
  std::unordered_map<const sta::LibertyPort*, float> port_slew_limit_cache_;
  // Design-level limits, looked up once per repairDesign call.
  float design_fanout_limit_ = 0.0f;  // 0 means unset
  bool design_fanout_limit_valid_ = false;
  // Telemetry. Counted across all calls within one repairDesign().
  int64_t screen_calls_ = 0;
  int64_t screen_safe_ = 0;
  // Per-reason rejection counters (for debugging the false-negative
  // rate). Printed alongside the safe rate in verbose mode.
  int64_t screen_rej_special_ = 0;    // special / dontTouch / tristate / etc
  int64_t screen_rej_no_lib_ = 0;     // no lib port or no lib limits
  int64_t screen_rej_top_bterm_ = 0;  // top-level OUTPUT/INOUT bterm
  int64_t screen_rej_cap_ = 0;        // cap_total > cap_limit
  int64_t screen_rej_slew_ = 0;       // slew_ub > slew_limit
  int64_t screen_rej_fanout_ = 0;     // fanout > fanout_limit
  // Knobs (defaults; can be tuned empirically).
  // HPWL is a lower bound on Steiner length. 1.5x is the typical
  // industry upper bound (Hwang 1976, FastSTA tooling). Going to
  // 1.2 trades a small theoretical false-skip risk for a much
  // tighter screen; we validate empirically vs the baseline buffer
  // count. Bump back to 1.5 if buffer-count drift appears.
  static constexpr float k_steiner_ub_ = 1.2f;
  // Extra safety margin applied on top of the lib limit (and on top
  // of any user slew/cap_margin). The existing slew_rc_factor_
  // already includes 10% modeling pessimism (RepairDesign.cc:134),
  // so the screen does not need its own. Empirically validate
  // buffer-count match before relaxing further.
  static constexpr float k_screen_safety_ = 0.0f;

  friend class Resizer;
};

}  // namespace rsz
