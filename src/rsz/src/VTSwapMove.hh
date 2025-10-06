// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"

namespace rsz {

class VTSwapSpeedMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  bool doMove(Instance* drvr, std::unordered_set<Instance*>& notSwappable);

  const char* name() override { return "VTSwapSpeed"; }

 private:
  bool isSwappable(const Path*& drvr_path,
                   Pin*& drvr_pin,
                   Instance*& drvr,
                   LibertyCell*& drvr_cell,
                   LibertyCell*& best_cell);
};

}  // namespace rsz
