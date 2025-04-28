// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include "BufferedNet.hh"
#include "PreChecks.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/GraphClass.hh"
#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

class Resizer;
enum class ParasiticsSrc;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Pin;
using sta::PinSeq;
using sta::Slew;
using sta::StaState;
using sta::Vertex;

using odb::Rect;

// Region for partioning fanout pins.
class LoadRegion
{
 public:
  LoadRegion();
  LoadRegion(PinSeq& pins, Rect& bbox);

  PinSeq pins_;
  Rect bbox_;  // dbu
  std::vector<LoadRegion> regions_;
};

class RepairDesign : dbStaState
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
  void repairNet(Net* net,
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

  bool getCin(const Pin* drvr_pin, float& cin);
  bool getLargestSizeCin(const Pin* drvr_pin, float& cin);
  void findBufferSizes();
  bool performGainBuffering(Net* net, const Pin* drvr_pin, int max_fanout);
  void performEarlySizingRound(int& repaired_net_count);

  void checkDriverArcSlew(const Corner* corner,
                          const Instance* inst,
                          const TimingArc* arc,
                          float load_cap,
                          float limit,
                          float& violation);
  bool repairDriverSlew(const Corner* corner, const Pin* drvr_pin);

  void repairNet(Net* net,
                 const Pin* drvr_pin,
                 Vertex* drvr,
                 bool check_slew,
                 bool check_cap,
                 bool check_fanout,
                 int max_length,  // dbu
                 bool resize_drvr,
                 int& repaired_net_count,
                 int& slew_violations,
                 int& cap_violations,
                 int& fanout_violations,
                 int& length_violations);
  bool needRepairCap(const Pin* drvr_pin,
                     int& cap_violations,
                     float& max_cap,
                     const Corner*& corner);
  bool needRepairWire(int max_length, int wire_length, int& length_violations);
  void checkSlew(const Pin* drvr_pin,
                 // Return values.
                 Slew& slew,
                 float& limit,
                 float& slack,
                 const Corner*& corner);
  float bufferInputMaxSlew(LibertyCell* buffer, const Corner* corner) const;
  void repairNet(const BufferedNetPtr& bnet,
                 const Pin* drvr_pin,
                 float max_cap,
                 int max_length,  // dbu
                 const Corner* corner);
  void repairNet(const BufferedNetPtr& bnet,
                 int level,
                 // Return values.
                 int& wire_length,
                 PinSeq& load_pins);
  void checkSlewLimit(float ref_cap, float max_load_slew);
  void repairNetWire(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     PinSeq& load_pins);
  void repairNetJunc(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     PinSeq& load_pins);
  void repairNetLoad(const BufferedNetPtr& bnet,
                     int level,
                     // Return values.
                     int& wire_length,
                     PinSeq& load_pins);
  float maxSlewMargined(float max_slew);
  double findSlewLoadCap(LibertyPort* drvr_port,
                         double slew,
                         const Corner* corner);
  double gateSlewDiff(LibertyPort* drvr_port,
                      double load_cap,
                      double slew,
                      const DcalcAnalysisPt* dcalc_ap);
  LoadRegion findLoadRegions(const Net* net,
                             const Pin* drvr_pin,
                             int max_fanout);
  void subdivideRegion(LoadRegion& region, int max_fanout);
  void makeRegionRepeaters(LoadRegion& region,
                           int max_fanout,
                           int level,
                           const Pin* drvr_pin,
                           bool check_slew,
                           bool check_cap,
                           int max_length,
                           bool resize_drvr);
  void makeFanoutRepeater(PinSeq& repeater_loads,
                          PinSeq& repeater_inputs,
                          const Rect& bbox,
                          const Point& loc,
                          bool check_slew,
                          bool check_cap,
                          int max_length,
                          bool resize_drvr);
  PinSeq findLoads(const Pin* drvr_pin);
  Rect findBbox(PinSeq& pins);
  Point findClosedPinLoc(const Pin* drvr_pin, PinSeq& pins);
  bool isRepeater(const Pin* load_pin);
  bool makeRepeater(const char* reason,
                    const Point& loc,
                    LibertyCell* buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    PinSeq& load_pins,
                    float& repeater_cap,
                    float& repeater_fanout,
                    float& repeater_max_slew);
  bool makeRepeater(const char* reason,
                    int x,
                    int y,
                    LibertyCell* buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    PinSeq& load_pins,
                    float& repeater_cap,
                    float& repeater_fanout,
                    float& repeater_max_slew,
                    Net*& out_net,
                    Pin*& repeater_in_pin,
                    Pin*& repeater_out_pin);
  LibertyCell* findBufferUnderSlew(float max_slew, float load_cap);
  bool hasInputPort(const Net* net);
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;

  void printProgress(int iteration,
                     bool force,
                     bool end,
                     int repaired_net_count) const;

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  PreChecks* pre_checks_ = nullptr;
  Resizer* resizer_;
  int dbu_ = 0;
  double initial_design_area_ = 0;
  ParasiticsSrc parasitics_src_ = ParasiticsSrc::none;

  // Gain buffering
  std::vector<LibertyCell*> buffer_sizes_;

  // Implicit arguments to repairNet bnet recursion.
  const Pin* drvr_pin_ = nullptr;
  float max_cap_ = 0;
  int max_length_ = 0;
  double slew_margin_ = 0;
  double cap_margin_ = 0;
  const Corner* corner_ = nullptr;

  int resize_count_ = 0;
  int inserted_buffer_count_ = 0;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  int print_interval_ = 0;
  std::shared_ptr<ResizerObserver> graphics_;

  // Elmore factor for 20-80% slew thresholds.
  static constexpr float elmore_skew_factor_ = 1.39;
  static constexpr int min_print_interval_ = 10;
  static constexpr int max_print_interval_ = 1000;
};

}  // namespace rsz
