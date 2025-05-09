// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <cmath>

#include "BaseMove.hh"

namespace rsz {

class SizeMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SizeMove"; }

 private:
  LibertyCell* upsizeCell(LibertyPort* in_port,
                          LibertyPort* drvr_port,
                          float load_cap,
                          float prev_drive,
                          const DcalcAnalysisPt* dcalc_ap);
  bool replaceCell(Instance* inst, const LibertyCell* replacement);
};

}  // namespace rsz
