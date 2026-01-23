// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "BufferedNet.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/geom.h"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;
class BufferedNet;
class RepairSetup;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;

class Rebuffer : public sta::dbStaState
{
 public:
  Rebuffer(Resizer* resizer);
  void fullyRebuffer(sta::Pin* user_pin = nullptr);

 protected:
  void init();
  void initOnCorner(sta::Corner* corner);

  void annotateLoadSlacks(BufferedNetPtr& tree, sta::Vertex* root_vertex);
  void annotateTiming(const BufferedNetPtr& tree);
  BufferedNetPtr stripTreeBuffers(const BufferedNetPtr& tree);
  BufferedNetPtr resteiner(const BufferedNetPtr& tree);
  BufferedNetPtr bufferForTiming(const BufferedNetPtr& tree,
                                 bool allow_topology_rewrite = true);
  BufferedNetPtr recoverArea(const BufferedNetPtr& root,
                             FixedDelay slack_target,
                             float alpha);

  void setPin(sta::Pin* drvr_pin);

  std::tuple<sta::Delay, sta::Delay, sta::Slew> drvrPinTiming(
      const BufferedNetPtr& bnet);
  FixedDelay slackAtDriverPin(const BufferedNetPtr& bnet);
  std::optional<FixedDelay> evaluateOption(const BufferedNetPtr& option,
                                           // Only used for debug print.
                                           int index);

  float maxSlewMargined(float max_slew);
  float maxCapMargined(float max_cap);

  bool loadSlewSatisfactory(sta::LibertyPort* driver,
                            const BufferedNetPtr& bnet);

  FixedDelay bufferDelay(sta::LibertyCell* cell,
                         const sta::RiseFallBoth* rf,
                         float load_cap);

  BufferedNetPtr addWire(const BufferedNetPtr& p,
                         odb::Point wire_end,
                         int wire_layer,
                         int level = -1);

  BufferedNetPtr importBufferTree(const sta::Pin* drvr_pin,
                                  const sta::Corner* corner);

  void insertBufferOptions(BufferedNetSeq& opts,
                           int level,
                           int next_segment_wl = 0,
                           bool area_oriented = false,
                           FixedDelay slack_threshold = FixedDelay::ZERO,
                           BufferedNet* exemplar = nullptr);

  void insertAssuredOption(BufferedNetSeq& opts,
                           BufferedNetPtr assured_opt,
                           int level);

  std::vector<sta::Instance*> collectImportedTreeBufferInstances(
      sta::Pin* drvr_pin,
      const BufferedNetPtr& imported_tree);
  int exportBufferTree(const BufferedNetPtr& choice,
                       sta::Net* net,  // output of buffer.
                       int level,
                       sta::Instance* parent_in,
                       const char* instance_base_name);

  void printProgress(int iteration, bool force, bool end, int remaining) const;

  int fanout(sta::Vertex* vertex) const;
  int wireLengthLimitImpliedByLoadSlew(sta::LibertyCell*);
  int wireLengthLimitImpliedByMaxCap(sta::LibertyCell*);

  BufferedNetPtr attemptTopologyRewrite(const BufferedNetPtr& node,
                                        const BufferedNetPtr& left,
                                        const BufferedNetPtr& right,
                                        float best_cap);

  float findBufferLoadLimitImpliedByDriverSlew(sta::LibertyCell* cell);
  void characterizeBufferLimits();

  struct BufferSize
  {
    sta::LibertyCell* cell;
    FixedDelay intrinsic_delay;
    float margined_max_cap;
    float driver_resistance;
  };

  bool bufferSizeCanDriveLoad(const BufferSize& size,
                              const BufferedNetPtr& bnet,
                              int extra_wire_length = 0);

  bool hasTopLevelOutputPort(sta::Net* net);
  int rebufferPin(const sta::Pin* drvr_pin);

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;

  std::vector<BufferSize> buffer_sizes_;
  std::map<sta::LibertyCell*, BufferSize*> buffer_sizes_index_;

  sta::Pin* pin_ = nullptr;
  float fanout_limit_ = 0.0f;
  float drvr_pin_max_slew_ = 0.0f;
  float drvr_load_high_water_mark_ = 0.0f;
  const sta::Corner* corner_ = nullptr;
  sta::LibertyPort* drvr_port_ = nullptr;
  sta::Path* arrival_paths_[sta::RiseFall::index_count] = {nullptr};

  int resizer_max_wire_length_ = 0;
  int wire_length_step_ = 0;

  double initial_design_area_ = 0.0;
  int print_interval_ = 0;
  int removed_count_ = 0;
  int inserted_count_ = 0;

  // margins in percent
  float slew_margin_ = 20.0f;
  float cap_margin_ = 20.0f;

  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  // Elmore factor for 20-80% slew thresholds.
  static constexpr float elmore_skew_factor_ = 1.39;
  static constexpr float relaxation_factor_ = 0.01;

  double long_wire_stepping_runtime_ = 0;

  friend class RepairSetup;
  friend class BufferMove;
};

};  // namespace rsz
