// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"

namespace rsz {

 using std::string;

 using sta::ArcDelay;
 using sta::InstancePinIterator;
 using sta::LoadPinIndexMap;
 using sta::NetConnectedPinIterator;
 using sta::Path;
 using sta::PathExpanded;
 using sta::Slack;
 using sta::Slew;

class SplitLoadMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() const override { return "SplitLoadMove"; }

 private:
};

}  // namespace rsz
