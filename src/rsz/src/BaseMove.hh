// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/StaState.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"
#include "sta/UnorderedMap.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;
using std::vector;

using odb::dbMaster;

using odb::dbMaster;
using odb::Point;

using utl::Logger;

using sta::ArcDelay;
using sta::Cell;
using sta::Corner;
using sta::dbDatabase;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::InstancePinIterator;
using sta::InstanceSet;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Network;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFall;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;
using InputSlews = std::array<Slew, RiseFall::index_count>;
using TgtSlews = std::array<Slew, RiseFall::index_count>;

struct SlackEstimatorParams
{
  SlackEstimatorParams(const float margin, const Corner* corner)
      : setup_slack_margin(margin), corner(corner)
  {
  }

  Pin* driver_pin{nullptr};
  Pin* prev_driver_pin{nullptr};
  Pin* driver_input_pin{nullptr};
  Instance* driver{nullptr};
  const Path* driver_path{nullptr};
  const Path* prev_driver_path{nullptr};
  LibertyCell* driver_cell{nullptr};
  const float setup_slack_margin;
  const Corner* corner;
};

class BaseMove : public sta::dbStaState
{
 public:
  BaseMove(Resizer* resizer);
  ~BaseMove() override = default;

  virtual bool doMove(const Path* drvr_path,
                      int drvr_index,
                      Slack drvr_slack,
                      PathExpanded* expanded,
                      float setup_slack_margin)
  {
    return false;
  }

  virtual const char* name() = 0;

  void init();

  // Accept the pending optimizations
  void commitMoves();
  // Abandon the pending optimizations
  void undoMoves();
  // Total pending optimizations (since last checkpoint)
  int numPendingMoves() const;
  // Whether this optimization is pending
  int hasPendingMoves(Instance* inst) const;
  // Total optimizations
  int numCommittedMoves() const;
  // Whether this optimization is committed or pending
  int hasMoves(Instance* inst) const;
  // Total accepted and pending optimizations
  int numMoves() const;
  // Add a new pending optimization
  void addMove(Instance* inst, int count = 1);

 protected:
  Resizer* resizer_;
  Logger* logger_;
  Network* network_;
  dbNetwork* db_network_;
  dbSta* sta_;
  dbDatabase* db_ = nullptr;
  int dbu_ = 0;
  dpl::Opendp* opendp_ = nullptr;
  const Corner* corner_ = nullptr;

  // Need to track these so we don't optimize the optimzations.
  // This can result in long run-time.
  // These are all of the optimized insts of this type .
  // Some may not have been accepted, but this replicates the prior behavior.
  InstanceSet all_inst_set_;
  // This is just the set of the pending moves.
  InstanceSet pending_inst_set_;
  int pending_count_ = 0;
  int all_count_ = 0;

  // Use actual input slews for accurate delay/slew estimation
  sta::UnorderedMap<LibertyPort*, InputSlews> input_slew_map_;
  TgtSlews tgt_slews_;

  double area(Cell* cell);
  double area(dbMaster* master);
  double dbuToMeters(int dist) const;

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

  bool isPortEqiv(sta::FuncExpr* expr,
                  const LibertyCell* cell,
                  const LibertyPort* port_a,
                  const LibertyPort* port_b);

  bool simulateExpr(
      sta::FuncExpr* expr,
      sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus,
      size_t table_index);
  std::vector<bool> simulateExpr(
      sta::FuncExpr* expr,
      sta::UnorderedMap<const LibertyPort*, std::vector<bool>>& port_stimulus);
  Instance* makeBuffer(LibertyCell* cell,
                       const char* name,
                       Instance* parent,
                       const Point& loc);
  bool estimatedSlackOK(const SlackEstimatorParams& params);
  bool estimateInputSlewImpact(Instance* instance,
                               const DcalcAnalysisPt* dcalc_ap,
                               Slew old_in_slew[RiseFall::index_count],
                               Slew new_in_slew[RiseFall::index_count],
                               // delay adjustment from prev stage
                               float delay_adjust,
                               SlackEstimatorParams params,
                               bool accept_if_slack_improves);
  bool hasPort(const Net* net);
  void getBufferPins(Instance* buffer, Pin*& ip, Pin*& op);
  int fanout(Vertex* vertex);

  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr int buffer_removal_max_fanout_ = 10;
  static constexpr float rebuffer_relaxation_factor_ = 0.03;
};

}  // namespace rsz
