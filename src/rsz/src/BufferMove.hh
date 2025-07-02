// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {

using std::string;

using sta::ArcDelay;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::RiseFallBoth;
using sta::Slack;
using sta::Slew;
using sta::Vertex;

class BufferMove : public BaseMove
{
 public:
  BufferMove(Resizer* resizer);

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "BufferMove"; }

  void rebufferNet(const Pin* drvr_pin);

 private:
  int rebuffer(const Pin* drvr_pin);

  void debugCheckMultipleBuffers(Path* path, PathExpanded* expanded);
  bool hasTopLevelOutputPort(Net* net);
};

}  // namespace rsz
