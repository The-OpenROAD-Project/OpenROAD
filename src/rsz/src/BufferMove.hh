// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class BufferMove : public BaseMove
{
 public:
  BufferMove(Resizer* resizer);

  bool doMove(const sta::Path* drvr_path,
              int drvr_index,
              sta::Slack drvr_slack,
              sta::PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() override { return "BufferMove"; }

  void rebufferNet(const sta::Pin* drvr_pin);

 private:
  int rebuffer(const sta::Pin* drvr_pin);

  void debugCheckMultipleBuffers(sta::Path* path, sta::PathExpanded* expanded);
  bool hasTopLevelOutputPort(sta::Net* net);
};

}  // namespace rsz
