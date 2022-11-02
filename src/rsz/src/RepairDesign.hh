/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "BufferedNet.hh"

#include "utl/Logger.h"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"

#include "sta/LibertyClass.hh"
#include "sta/NetworkClass.hh"
#include "sta/GraphClass.hh"
#include "sta/Delay.hh"
#include "sta/Corner.hh"

namespace rsz {

class Resizer;
class FanoutRender;

using std::vector;

using sta::StaState;
using sta::Net;
using sta::Pin;
using sta::PinSeq;
using sta::Vertex;
using sta::Slew;
using sta::Corner;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::dbNetwork;
using sta::dbSta;
using sta::MinMax;
using sta::Vector;

using odb::Rect;

// Region for partioning fanout pins.
class LoadRegion
{
public:
  LoadRegion();
  LoadRegion(Vector<Pin*> &pins,
       Rect &bbox);

  Vector<Pin*> pins_;
  Rect bbox_; // dbu
  vector<LoadRegion> regions_;
};

class RepairDesign : StaState
{
public:
  RepairDesign(Resizer *resizer);
  ~RepairDesign();
  void repairDesign(double max_wire_length,
                    double slew_margin,
                    double cap_margin);
  void repairDesign(double max_wire_length, // zero for none (meters)
                    double slew_margin,
                    double cap_margin,
                    int &repair_count,
                    int &slew_violations,
                    int &cap_violations,
                    int &fanout_violations,
                    int &length_violations);
  int insertedBufferCount() const { return inserted_buffer_count_; }
  void repairNet(Net *net,
                 double max_wire_length,
                 double slew_margin,
                 double cap_margin);
  void repairClkNets(double max_wire_length);
  void repairClkInverters();

protected:
  void init();
  void repairNet(Net *net,
                 const Pin *drvr_pin,
                 Vertex *drvr,
                 bool check_slew,
                 bool check_cap,
                 bool check_fanout,
                 int max_length, // dbu
                 bool resize_drvr,
                 int &repair_count,
                 int &slew_violations,
                 int &cap_violations,
                 int &fanout_violations,
                 int &length_violations);
  bool checkLimits(const Pin *drvr_pin,
                   bool check_slew,
                   bool check_cap,
                   bool check_fanout);
  void checkSlew(const Pin *drvr_pin,
                 // Return values.
                 Slew &slew,
                 float &limit,
                 float &slack,
                 const Corner *&corner);
  float bufferInputMaxSlew(LibertyCell *buffer,
                           const Corner *corner) const;
  void repairNet(BufferedNetPtr bnet,
                 const Pin *drvr_pin,
                 float max_cap,
                 int max_length, // dbu
                 const Corner *corner);
  void repairNet(BufferedNetPtr bnet,
                 int level,
                 // Return values.
                 int &wire_length,
                 PinSeq &load_pins);
  void repairNetWire(BufferedNetPtr bnet,
                     int level,
                     // Return values.
                     int &wire_length,
                     PinSeq &load_pins);
  void repairNetJunc(BufferedNetPtr bnet,
                     int level,
                     // Return values.
                     int &wire_length,
                     PinSeq &load_pins);
  void repairNetLoad(BufferedNetPtr bnet,
                     int level,
                     // Return values.
                     int &wire_length,
                     PinSeq &load_pins);
  float maxSlewMargined(float max_slew);
  double findSlewLoadCap(LibertyPort *drvr_port,
                         double slew,
                         const Corner *corner);
  double gateSlewDiff(LibertyPort *drvr_port,
                      double load_cap,
                      double slew,
                      const DcalcAnalysisPt *dcalc_ap);
  LoadRegion findLoadRegions(const Pin *drvr_pin,
                             int max_fanout);
  void subdivideRegion(LoadRegion &region,
                       int max_fanout);
  void makeRegionRepeaters(LoadRegion &region,
                           int max_fanout,
                           int level,
                           const Pin *drvr_pin,
                           bool check_slew,
                           bool check_cap,
                           int max_length,
                           bool resize_drvr);
  void makeFanoutRepeater(PinSeq &repeater_loads,
                          PinSeq &repeater_inputs,
                          Rect bbox,
                          Point loc,
                          bool check_slew,
                          bool check_cap,
                          int max_length,
                          bool resize_drvr);
  PinSeq findLoads(const Pin *drvr_pin);
  Rect findBbox(PinSeq &pins);
  Point findClosedPinLoc(const Pin *drvr_pin,
                         PinSeq &pins);
  bool isRepeater(const Pin *load_pin);
  void makeRepeater(const char *reason,
                    Point loc,
                    LibertyCell *buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    PinSeq &load_pins,
                    float &repeater_cap,
                    float &repeater_fanout,
                    float &repeater_max_slew);
  void makeRepeater(const char *reason,
                    int x,
                    int y,
                    LibertyCell *buffer_cell,
                    bool resize,
                    int level,
                    // Return values.
                    PinSeq &load_pins,
                    float &repeater_cap,
                    float &repeater_fanout,
                    float &repeater_max_slew,
                    Net *&out_net,
                    Pin *&repeater_in_pin,
                    Pin *&repeater_out_pin);
  LibertyCell *findBufferUnderSlew(float max_slew,
                                   float load_cap);
  float bufferSlew(LibertyCell *buffer_cell,
                   float load_cap,
                   const DcalcAnalysisPt *dcalc_ap);
  bool hasInputPort(const Net *net);
  double dbuToMeters(int dist) const;
  int metersToDbu(double dist) const;
  double dbuToMicrons(int dist) const;

  Logger *logger_;
  dbSta *sta_;
  dbNetwork *db_network_;
  Resizer *resizer_;
  int dbu_;

  // Implicit arguments to repairNet bnet recursion.
  const Pin *drvr_pin_;
  float max_cap_;
  int max_length_;
  double slew_margin_;
  double cap_margin_;
  const Corner *corner_;

  int resize_count_;
  int inserted_buffer_count_;
  const MinMax *min_;
  const MinMax *max_;

  FanoutRender *fanout_render_;

  // Elmore factor for 20-80% slew thresholds.
  static constexpr float elmore_skew_factor_ = 1.39;

  friend class FanoutRender;
};

class FanoutRender : public gui::Renderer
{
public:
  FanoutRender(RepairDesign *repair);
  void setRect(Rect &rect);
  void setDrvrLoc(Point loc);
  void setPins(PinSeq *pins);
  void drawObjects(gui::Painter &painter);

private:
  RepairDesign *repair_;
  Rect rect_;
  PinSeq *pins_;
  Point drvr_loc_;
};

} // namespace
