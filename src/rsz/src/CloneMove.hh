// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {

using std::pair;
using std::string;
using std::vector;

using odb::Point;

using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

class CloneMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "CloneMove"; }

 private:
  Point computeCloneGateLocation(
      const Pin* drvr_pin,
      const std::vector<std::pair<Vertex*, Slack>>& fanout_slacks);

  bool cloneDriver(const Path* drvr_path,
                   int drvr_index,
                   Slack drvr_slack,
                   PathExpanded* expanded);
};

}  // namespace rsz
