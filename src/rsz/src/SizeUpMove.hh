// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "sta/Delay.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

class SizeUpMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SizeUpMove"; }
};

// Upsize cells to match drive strength with previous stage
class SizeUpMatchMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SizeUpMoveMatch"; }
};

}  // namespace rsz
