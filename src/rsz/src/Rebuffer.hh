// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"

namespace rsz {

using utl::Logger;

using odb::Point;
using sta::Corner;
using sta::dbNetwork;
using sta::Delay;
using sta::Instance;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::MinMax;
using sta::Net;
using sta::Path;
using sta::Pin;
using sta::RiseFall;
using sta::RiseFallBoth;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

class Resizer;
class BufferedNet;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;

class Rebuffer : public sta::dbStaState
{
 public:
  Rebuffer(Resizer* resizer);
  void fullyRebuffer(Pin* user_pin = nullptr);

 protected:
  void init();

  void annotateLoadSlacks(BufferedNetPtr& tree, Vertex* root_vertex);
  void annotateTiming(const BufferedNetPtr& tree);
  BufferedNetPtr stripTreeBuffers(const BufferedNetPtr& tree);
  BufferedNetPtr resteiner(const BufferedNetPtr& tree);
  BufferedNetPtr bufferForTiming(const BufferedNetPtr& tree);
  BufferedNetPtr recoverArea(const BufferedNetPtr& root,
                             Delay slack_target,
                             float alpha);

  std::tuple<Delay, Delay, Slew> drvrPinTiming(const BufferedNetPtr& bnet);
  Slack slackAtDriverPin(const BufferedNetPtr& bnet);
  std::optional<Slack> evaluateOption(const BufferedNetPtr& option,
                                      // Only used for debug print.
                                      int index);

  float maxSlewMargined(float max_slew);
  float maxCapMargined(float max_cap);

  bool loadSlewSatisfactory(LibertyPort* driver, const BufferedNetPtr& bnet);

  Delay bufferDelay(LibertyCell* cell, const RiseFallBoth* rf, float load_cap);

  BufferedNetPtr addWire(const BufferedNetPtr& p,
                         Point wire_end,
                         int wire_layer,
                         int level = -1);

  BufferedNetPtr importBufferTree(const Pin* drvr_pin, const Corner* corner);

  void insertBufferOptions(BufferedNetSeq& opts,
                           int level,
                           int next_segment_wl = 0,
                           bool area_oriented = false,
                           Delay slack_threshold = 0,
                           BufferedNet* exemplar = nullptr);

  void insertAssuredOption(BufferedNetSeq& opts,
                           BufferedNetPtr assured_opt,
                           int level);

  std::vector<Instance*> collectImportedTreeBufferInstances(
      Pin* drvr_pin,
      const BufferedNetPtr& imported_tree);
  int exportBufferTree(const BufferedNetPtr& choice,
                       Net* net,  // output of buffer.
                       int level,
                       Instance* parent_in,
                       odb::dbITerm* mod_net_drvr,
                       odb::dbModNet* mod_net_in);

  void printProgress(int iteration, bool force, bool end, int remaining) const;

  Delay marginedSlackThreshold(Delay slack);

  int fanout(Vertex* vertex) const;
  int wireLengthLimitImpliedByLoadSlew(LibertyCell*);
  int wireLengthLimitImpliedByMaxCap(LibertyCell*);

  BufferedNetPtr attemptTopologyRewrite(const BufferedNetPtr& node,
                                        const BufferedNetPtr& left,
                                        const BufferedNetPtr& right,
                                        float best_cap);

  float findBufferLoadLimitImpliedByDriverSlew(LibertyCell* cell);
  void characterizeBufferLimits();

  struct BufferSize
  {
    LibertyCell* cell;
    Delay intrinsic_delay;
    float margined_max_cap;
  };

  bool bufferSizeCanDriveLoad(const BufferSize& size,
                              const BufferedNetPtr& bnet,
                              int extra_wire_length = 0);

  Logger* logger_ = nullptr;
  dbNetwork* db_network_ = nullptr;
  Resizer* resizer_;

  std::vector<BufferSize> buffer_sizes_;
  std::map<LibertyCell*, BufferSize*> buffer_sizes_index_;

  Pin* pin_;
  float fanout_limit_;
  float drvr_pin_max_slew_;
  float drvr_load_high_water_mark_;
  const Corner* corner_ = nullptr;
  LibertyPort* drvr_port_ = nullptr;
  Path* arrival_paths_[RiseFall::index_count];
  Delay ref_slack_;

  int resizer_max_wire_length_;
  int wire_length_step_;

  double initial_design_area_;
  int print_interval_;
  int removed_count_;
  int inserted_count_;

  // margins in percent
  float slew_margin_ = 20.0f;
  float cap_margin_ = 20.0f;

  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();

  // Elmore factor for 20-80% slew thresholds.
  static constexpr float elmore_skew_factor_ = 1.39;
  static constexpr float relaxation_factor_ = 0.01;
};

};  // namespace rsz
