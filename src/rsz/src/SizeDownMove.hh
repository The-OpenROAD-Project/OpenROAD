// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace rsz {

class SizeDownMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SizeDownMove"; }

 private:
  sta::LibertyCell* downSizeGate(const sta::LibertyPort* drvr_port,
                                 const sta::LibertyPort* load_port,
                                 const sta::Pin* load_pin,
                                 const sta::DcalcAnalysisPt* dcalc_ap,
                                 float slack_margin);
};

}  // namespace rsz
