// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Delay.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;

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
  sta::Vertex* recoverPower(const sta::Pin* end_pin);

 private:
  void init();
  sta::Vertex* recoverPower(const sta::Path* path, sta::Slack path_slack);
  bool meetsSizeCriteria(const sta::LibertyCell* cell,
                         const sta::LibertyCell* candidate,
                         bool match_size);
  bool downsizeDrvr(const sta::Path* drvr_path,
                    int drvr_index,
                    sta::PathExpanded* expanded,
                    bool only_same_size_swap,
                    sta::Slack path_slack);

  sta::LibertyCell* downsizeCell(const sta::LibertyPort* in_port,
                                 const sta::LibertyPort* drvr_port,
                                 float load_cap,
                                 float prev_drive,
                                 const sta::Scene* scene,
                                 const sta::MinMax* min_max,
                                 bool match_size,
                                 sta::Slack path_slack);
  int fanout(sta::Vertex* vertex);
  bool hasTopLevelOutputPort(sta::Net* net);

  BufferedNetSeq addWireAndBuffer(BufferedNetSeq Z,
                                  BufferedNetPtr bnet_wire,
                                  int level);
  float pinCapacitance(const sta::Pin* pin, const sta::Scene* scene);
  float bufferInputCapacitance(sta::LibertyCell* buffer_cell,
                               const sta::Scene* scene);
  sta::Slack slackPenalized(BufferedNetPtr bnet);
  sta::Slack slackPenalized(BufferedNetPtr bnet, int index);

  void printProgress(int iteration, bool force, bool end) const;

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
  const sta::Scene* scene_ = nullptr;
  int resize_count_ = 0;
  const sta::MinMax* max_ = sta::MinMax::max();

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
