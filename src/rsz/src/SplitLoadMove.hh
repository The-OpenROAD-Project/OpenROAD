// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"
#include "sta/Path.hh"

namespace rsz {

class SplitLoadMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path, float setup_slack_margin) override;

  const char* name() override { return "SplitLoadMove"; }
};

}  // namespace rsz
