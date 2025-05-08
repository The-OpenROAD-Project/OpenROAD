// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BaseMove.hh"

namespace rsz {


 using std::string;

 using odb::dbInst;

 using utl::RSZ;

 using sta::ArcDelay;
 using sta::Corner;
 using sta::DcalcAnalysisPt;
 using sta::GraphDelayCalc;
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

class UnbufferMove : public BaseMove
{
 public:
  using BaseMove::BaseMove;

  bool doMove(const Path* drvr_path,
              const int drvr_index,
              Slack drvr_slack,
              PathExpanded* expanded,
              float setup_slack_margin) override;

  const char* name() const override { return "UnbufferMove"; }

  bool removeBufferIfPossible(Instance* buffer, bool honorDontTouchFixed);

 private:
  void removeBuffer(Instance* buffer);
  bool canRemoveBuffer(Instance* buffer, bool honorDontTouchFixed);
  bool bufferBetweenPorts(Instance* buffer);

  static constexpr int buffer_removal_max_fanout_ = 10;
};

}  // namespace rsz
