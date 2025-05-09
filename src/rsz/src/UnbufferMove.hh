// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {

using std::string;

using sta::ArcDelay;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Slack;
using sta::Slew;

class UnbufferMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "UnbufferMove"; }

  bool removeBufferIfPossible(Instance* buffer, bool honorDontTouchFixed);

 private:
  void removeBuffer(Instance* buffer);
  bool canRemoveBuffer(Instance* buffer, bool honorDontTouchFixed);
  bool bufferBetweenPorts(Instance* buffer);

  static constexpr int buffer_removal_max_fanout_ = 10;
};

}  // namespace rsz
