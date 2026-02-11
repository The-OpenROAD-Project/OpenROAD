// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
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

namespace est {
class EstimateParasitics;
}

namespace rsz {

using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;
using InputSlews = std::array<sta::Slew, sta::RiseFall::index_count>;
using TgtSlews = std::array<sta::Slew, sta::RiseFall::index_count>;

struct SlackEstimatorParams
{
  SlackEstimatorParams(const float margin, const sta::Corner* corner)
      : setup_slack_margin(margin), corner(corner)
  {
  }

  sta::Pin* driver_pin{nullptr};
  sta::Pin* prev_driver_pin{nullptr};
  sta::Pin* driver_input_pin{nullptr};
  sta::Instance* driver{nullptr};
  const sta::Path* driver_path{nullptr};
  const sta::Path* prev_driver_path{nullptr};
  sta::LibertyCell* driver_cell{nullptr};
  const float setup_slack_margin;
  const sta::Corner* corner;
};

class BaseMove : public sta::dbStaState
{
 public:
  BaseMove(Resizer* resizer);
  ~BaseMove() override = default;

  virtual bool doMove(const sta::Path* drvr_path,
                      int drvr_index,
                      sta::Slack drvr_slack,
                      sta::PathExpanded* expanded,
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
  int hasPendingMoves(sta::Instance* inst) const;
  // Total optimizations
  int numCommittedMoves() const;
  // Total rejected count
  int numRejectedMoves() const;
  // Whether this optimization is committed or pending
  int hasMoves(sta::Instance* inst) const;
  // Total accepted and pending optimizations
  int numMoves() const;
  // Add a new pending optimization
  void addMove(sta::Instance* inst, int count = 1);

 protected:
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
  utl::Logger* logger_;
  sta::Network* network_;
  sta::dbNetwork* db_network_;
  sta::dbSta* sta_;
  odb::dbDatabase* db_ = nullptr;
  int dbu_ = 0;
  dpl::Opendp* opendp_ = nullptr;
  const sta::Corner* corner_ = nullptr;

  // Need to track these so we don't optimize the optimzations.
  // This can result in long run-time.
  // These are all of the optimized insts of this type .
  // Some may not have been accepted, but this replicates the prior behavior.
  sta::InstanceSet all_inst_set_;
  // This is just the set of the pending moves.
  sta::InstanceSet pending_inst_set_;
  int pending_count_ = 0;
  int all_count_ = 0;
  int rejected_count_ = 0;
  int accepted_count_ = 0;

  // Use actual input slews for accurate delay/slew estimation
  sta::UnorderedMap<sta::LibertyPort*, InputSlews> input_slew_map_;
  TgtSlews tgt_slews_;

  double area(sta::Cell* cell);
  double area(odb::dbMaster* master);

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

  bool isPortEqiv(sta::FuncExpr* expr,
                  const sta::LibertyCell* cell,
                  const sta::LibertyPort* port_a,
                  const sta::LibertyPort* port_b);

  bool simulateExpr(sta::FuncExpr* expr,
                    sta::UnorderedMap<const sta::LibertyPort*,
                                      std::vector<bool>>& port_stimulus,
                    size_t table_index);
  std::vector<bool> simulateExpr(
      sta::FuncExpr* expr,
      sta::UnorderedMap<const sta::LibertyPort*, std::vector<bool>>&
          port_stimulus);
  sta::Instance* makeBuffer(sta::LibertyCell* cell,
                            const char* name,
                            sta::Instance* parent,
                            const odb::Point& loc);
  bool estimatedSlackOK(const SlackEstimatorParams& params);
  bool estimateInputSlewImpact(
      sta::Instance* instance,
      const sta::DcalcAnalysisPt* dcalc_ap,
      sta::Slew old_in_slew[sta::RiseFall::index_count],
      sta::Slew new_in_slew[sta::RiseFall::index_count],
      // delay adjustment from prev stage
      float delay_adjust,
      SlackEstimatorParams params,
      bool accept_if_slack_improves);
  void getBufferPins(sta::Instance* buffer, sta::Pin*& ip, sta::Pin*& op);
  int fanout(sta::Vertex* vertex);

  sta::LibertyCell* upsizeCell(sta::LibertyPort* in_port,
                               sta::LibertyPort* drvr_port,
                               float load_cap,
                               float prev_drive,
                               const sta::DcalcAnalysisPt* dcalc_ap);
  bool replaceCell(sta::Instance* inst, const sta::LibertyCell* replacement);
  bool checkMaxCapViolation(sta::Instance* inst,
                            const sta::LibertyCell* replacement);
  float getInputPinCapacitance(sta::Pin* pin, const sta::LibertyCell* cell);
  bool checkMaxCapOK(const sta::Pin* drvr_pin, float cap_delta);

  bool checkMaxCapViolation(const sta::Pin* output_pin,
                            sta::LibertyPort* output_port,
                            float output_cap);
  bool checkMaxSlewViolation(const sta::Pin* output_pin,
                             sta::LibertyPort* output_port,
                             float output_slew_factor,
                             float output_cap,
                             const sta::DcalcAnalysisPt* dcalc_ap);
  float computeElmoreSlewFactor(const sta::Pin* output_pin,
                                sta::LibertyPort* output_port,
                                float output_load_cap);
  sta::ArcDelay getWorstIntrinsicDelay(const sta::LibertyPort* input_port);
  sta::Slack getWorstInputSlack(sta::Instance* inst);
  sta::Slack getWorstOutputSlack(sta::Instance* inst);
  std::vector<const sta::LibertyPort*> getOutputPorts(
      const sta::LibertyCell* cell);
  std::vector<const sta::Pin*> getOutputPins(const sta::Instance* inst);
  sta::LibertyCellSeq getSwappableCells(sta::LibertyCell* base);

  static constexpr int size_down_max_fanout_ = 10;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr int buffer_removal_max_fanout_ = 10;
  static constexpr float rebuffer_relaxation_factor_ = 0.03;
};

}  // namespace rsz
