// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <utility>
#include <vector>

#include "BaseMove.hh"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

class CloneMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "CloneMove"; }

 private:
  odb::Point computeCloneGateLocation(
      const sta::Pin* drvr_pin,
      const std::vector<std::pair<sta::Vertex*, sta::Slack>>& fanout_slacks);

  bool cloneDriver(const sta::Path* drvr_path,
                   int drvr_index,
                   sta::Slack drvr_slack,
                   sta::PathExpanded* expanded);
};

}  // namespace rsz
