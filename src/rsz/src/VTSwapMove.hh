// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>
#include <unordered_set>

#include "BaseMove.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

class VTSwapSpeedMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  bool doMove(sta::Instance* drvr,
              std::unordered_set<sta::Instance*>& notSwappable);

  const char* name() override { return "VTSwapSpeed"; }

 private:
  bool isSwappable(const sta::Path*& drvr_path,
                   sta::Pin*& drvr_pin,
                   sta::Instance*& drvr,
                   sta::LibertyCell*& drvr_cell,
                   sta::LibertyCell*& best_cell);
};

}  // namespace rsz
