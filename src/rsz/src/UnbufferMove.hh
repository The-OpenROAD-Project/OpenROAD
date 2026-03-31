// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"

namespace odb {
class dbModNet;
}

namespace rsz {

class UnbufferMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const sta::Path* drvr_path, float setup_slack_margin) override;

  const char* name() override { return "UnbufferMove"; }

  bool removeBufferIfPossible(sta::Instance* buffer, bool honorDontTouchFixed);
  bool canRemoveBuffer(sta::Instance* buffer, bool honorDontTouchFixed);
  bool removeBuffer(sta::Instance* buffer);

 private:
  bool bufferBetweenPorts(sta::Instance* buffer);
  // Returns true if removing buffer creates a feedthrough (input port
  // directly wired to output port within the same module).
  bool bufferRemovalCreatesFeedthrough(odb::dbModNet* ip_modnet,
                                       odb::dbModNet* op_modnet) const;

  static constexpr int buffer_removal_max_fanout_ = 10;
};

}  // namespace rsz
