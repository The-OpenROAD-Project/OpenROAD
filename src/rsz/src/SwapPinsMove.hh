// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <vector>

#include "BaseMove.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/UnorderedMap.hh"

namespace rsz {

class SwapPinsMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SwapPinsMove"; }

  void reportSwappablePins();

 private:
  using LibertyPortVec = std::vector<sta::LibertyPort*>;
  void swapPins(sta::Instance* inst,
                sta::LibertyPort* port1,
                sta::LibertyPort* port2);
  void equivCellPins(const sta::LibertyCell* cell,
                     sta::LibertyPort* input_port,
                     LibertyPortVec& ports);
  void annotateInputSlews(sta::Instance* inst,
                          const sta::DcalcAnalysisPt* dcalc_ap);
  void findSwapPinCandidate(sta::LibertyPort* input_port,
                            sta::LibertyPort* drvr_port,
                            const LibertyPortVec& equiv_ports,
                            float load_cap,
                            const sta::DcalcAnalysisPt* dcalc_ap,
                            sta::LibertyPort** swap_port);
  void resetInputSlews();

  sta::UnorderedMap<sta::LibertyPort*, LibertyPortVec> equiv_pin_map_;
};

}  // namespace rsz
