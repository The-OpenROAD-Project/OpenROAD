// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {

using std::string;

using sta::ArcDelay;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFallBoth;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

class BufferMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "BufferMove"; }

  void rebufferNet(const Pin* drvr_pin);

 private:
  int rebuffer_net_count_ = 0;
  LibertyPort* drvr_port_ = nullptr;

  int rebuffer(const Pin* drvr_pin);

  void annotateLoadSlacks(BufferedNetPtr& bnet, Vertex* root_vertex);
  BufferedNetPtr rebufferForTiming(const BufferedNetPtr& bnet);
  BufferedNetPtr recoverArea(const BufferedNetPtr& bnet,
                             sta::Delay slack_target,
                             float alpha);

  void debugCheckMultipleBuffers(Path* path, PathExpanded* expanded);
  bool hasTopLevelOutputPort(Net* net);

  int rebufferTopDown(const BufferedNetPtr& choice,
                      Net* net,
                      int level,
                      Instance* parent,
                      odb::dbITerm* mod_net_drvr,
                      odb::dbModNet* mod_net);
  BufferedNetPtr addWire(const BufferedNetPtr& p,
                         const Point& wire_end,
                         int wire_layer,
                         int level);
  void addBuffers(BufferedNetSeq& Z1,
                  int level,
                  bool area_oriented = false,
                  sta::Delay threshold = 0);
  float bufferInputCapacitance(LibertyCell* buffer_cell,
                               const DcalcAnalysisPt* dcalc_ap);
  Delay bufferDelay(LibertyCell* cell, const RiseFallBoth* rf, float load_cap);
  std::tuple<sta::Delay, sta::Delay> drvrPinTiming(const BufferedNetPtr& bnet);
  Slack slackAtDriverPin(const BufferedNetPtr& bnet);
  Slack slackAtDriverPin(const BufferedNetPtr& bnet, int index);

  Delay requiredDelay(const BufferedNetPtr& bnet);

  // For rebuffering
  Path* arrival_paths_[RiseFall::index_count];
};

}  // namespace rsz
