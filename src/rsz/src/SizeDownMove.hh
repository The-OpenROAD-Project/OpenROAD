// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

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

class SizeDownMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SizeDownMove"; }

 private:
  LibertyCell* downsizeFanout(const LibertyPort* drvr_port,
                              const Pin* drvr_pin,
                              const LibertyPort* fanout_port,
                              const Pin* fanout_pin,
                              const DcalcAnalysisPt* dcalc_ap,
                              float fanout_slack);
};

}  // namespace rsz
