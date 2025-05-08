// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"

namespace rsz {

 using std::pair;
 using std::string;
 using std::vector;

 using odb::Point;

 using utl::RSZ;

 using sta::ArcDelay;
 using sta::Edge;
 using sta::Instance;
 using sta::InstancePinIterator;
 using sta::LibertyCell;
 using sta::LibertyPort;
 using sta::LoadPinIndexMap;
 using sta::Net;
 using sta::NetConnectedPinIterator;
 using sta::Path;
 using sta::PathExpanded;
 using sta::Pin;
 using sta::RiseFall;
 using sta::Slack;
 using sta::Slew;
 using sta::Vertex;
 using sta::VertexOutEdgeIterator;

class SplitLoadMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              const int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() const override { return "SplitLoadMove"; }

 private:
};

}  // namespace rsz
