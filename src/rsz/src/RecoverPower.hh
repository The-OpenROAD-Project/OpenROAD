// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace sta {
class PathExpanded;
}

namespace rsz {

class Resizer;

using utl::Logger;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::DcalcAnalysisPt;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::StaState;
using sta::TimingArc;
using sta::Vertex;

class BufferedNet;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;

class RecoverPower : public sta::dbStaState
{
 public:
  RecoverPower(Resizer* resizer);
  bool recoverPower(float recover_power_percent, bool verbose);
  // For testing.
  Vertex* recoverPower(const Pin* end_pin);

 private:
  void init();
  Vertex* recoverPower(const Path* path, Slack path_slack);
  bool meetsSizeCriteria(const LibertyCell* cell,
                         const LibertyCell* candidate,
                         bool match_size);
  bool downsizeDrvr(const Path* drvr_path,
                    int drvr_index,
                    PathExpanded* expanded,
                    bool only_same_size_swap,
                    Slack path_slack);

  LibertyCell* downsizeCell(const LibertyPort* in_port,
                            const LibertyPort* drvr_port,
                            float load_cap,
                            float prev_drive,
                            const DcalcAnalysisPt* dcalc_ap,
                            bool match_size,
                            Slack path_slack);
  int fanout(Vertex* vertex);
  bool hasTopLevelOutputPort(Net* net);

  BufferedNetSeq addWireAndBuffer(BufferedNetSeq Z,
                                  BufferedNetPtr bnet_wire,
                                  int level);
  float pinCapacitance(const Pin* pin, const DcalcAnalysisPt* dcalc_ap);
  float bufferInputCapacitance(LibertyCell* buffer_cell,
                               const DcalcAnalysisPt* dcalc_ap);
  Slack slackPenalized(BufferedNetPtr bnet);
  Slack slackPenalized(BufferedNetPtr bnet, int index);

  void printProgress(int iteration, bool force, bool end) const;

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  const Corner* corner_ = nullptr;
  int resize_count_ = 0;
  const MinMax* max_ = MinMax::max();

  // Paths with slack more than this would be considered for power recovery
  static constexpr float setup_slack_margin_ = 1e-11;
  // For paths with no timing the max margin is INT_MAX. We need to filter those
  // out (using 1e-4)
  static constexpr float setup_slack_max_margin_ = 1e-4;
  // Threshold for failed successive moves for power recovery before we stop
  // trying
  static constexpr int failed_move_threshold_limit_ = 500;

  sta::VertexSet bad_vertices_;

  double initial_design_area_ = 0;
  int print_interval_ = 0;

  static constexpr int decreasing_slack_max_passes_ = 50;
  static constexpr int rebuffer_max_fanout_ = 20;
  static constexpr int split_load_min_fanout_ = 8;
  static constexpr double rebuffer_buffer_penalty_ = .01;

  static constexpr int min_print_interval_ = 10;
  static constexpr int max_print_interval_ = 100;
};

}  // namespace rsz
