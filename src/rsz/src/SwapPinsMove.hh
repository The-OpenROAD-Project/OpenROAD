// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include <vector>

#include "BaseMove.hh"
#include "sta/Liberty.hh"
#include "sta/UnorderedMap.hh"

namespace rsz {

using sta::ArcDelay;
using sta::DcalcAnalysisPt;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LibertyCell;
using sta::LibertyPort;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Slack;
using sta::Slew;

class SwapPinsMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "SwapPinsMove"; }

  void reportSwappablePins();

 private:
  using LibertyPortVec = std::vector<LibertyPort*>;
  void swapPins(Instance* inst, LibertyPort* port1, LibertyPort* port2);
  void equivCellPins(const LibertyCell* cell,
                     LibertyPort* input_port,
                     LibertyPortVec& ports);
  void annotateInputSlews(Instance* inst, const DcalcAnalysisPt* dcalc_ap);
  void findSwapPinCandidate(LibertyPort* input_port,
                            LibertyPort* drvr_port,
                            const LibertyPortVec& equiv_ports,
                            float load_cap,
                            const DcalcAnalysisPt* dcalc_ap,
                            LibertyPort** swap_port);
  void resetInputSlews();

  sta::UnorderedMap<LibertyPort*, LibertyPortVec> equiv_pin_map_;
};

}  // namespace rsz
