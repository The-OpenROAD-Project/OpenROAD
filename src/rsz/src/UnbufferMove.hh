// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"
#include "sta/Delay.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

class UnbufferMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "UnbufferMove"; }

  bool removeBufferIfPossible(sta::Instance* buffer, bool honorDontTouchFixed);
  bool canRemoveBuffer(sta::Instance* buffer, bool honorDontTouchFixed);
  bool removeBuffer(sta::Instance* buffer);

 private:
  bool bufferBetweenPorts(sta::Instance* buffer);

  static constexpr int buffer_removal_max_fanout_ = 10;
};

}  // namespace rsz
