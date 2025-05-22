// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BufferMove.hh"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "BaseMove.hh"

namespace rsz {

using std::string;

using utl::RSZ;

using sta::ArcDelay;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::Net;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;
using sta::TimingArc;
using sta::Vertex;

bool BufferMove::doMove(const Path* drvr_path,
                        int drvr_index,
                        Slack drvr_slack,
                        PathExpanded* expanded,
                        float setup_slack_margin)
{
  Vertex* drvr_vertex = drvr_path->vertex(sta_);
  const Pin* drvr_pin = drvr_vertex->pin();
  Instance* drvr_inst = network_->instance(drvr_pin);

  const int fanout = this->fanout(drvr_vertex);
  if (fanout <= 1) {
    return false;
  }
  // Rebuffer blows up on large fanout nets.
  if (fanout >= rebuffer_max_fanout_) {
    return false;
  }
  const bool tristate_drvr = resizer_->isTristateDriver(drvr_pin);
  if (tristate_drvr) {
    return false;
  }
  const Net* net = db_network_->dbToSta(db_network_->flatNet(drvr_pin));
  if (resizer_->dontTouch(net)) {
    return false;
  }
  dbNet* db_net = db_network_->staToDb(net);
  if (db_net->isConnectedByAbutment()) {
    return false;
  }

  const int rebuffer_count = rebuffer(drvr_pin);
  if (rebuffer_count > 0) {
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               3,
               "rebuffer {} inserted {}",
               network_->pathName(drvr_pin),
               rebuffer_count);
    debugPrint(logger_,
               RSZ,
               "moves",
               1,
               "rebuffer {} inserted {}",
               network_->pathName(drvr_pin),
               rebuffer_count);
    addMove(drvr_inst, rebuffer_count);
  }
  return rebuffer_count > 0;
}

void BufferMove::debugCheckMultipleBuffers(Path* path, PathExpanded* expanded)
{
  if (expanded->size() > 1) {
    const int path_length = expanded->size();
    const int start_index = expanded->startIndex();
    for (int i = start_index; i < path_length; i++) {
      const Path* path = expanded->path(i);
      const Pin* path_pin = path->pin(sta_);
      if (i > 0 && network_->isDriver(path_pin)
          && !network_->isTopLevelPort(path_pin)) {
        const TimingArc* prev_arc = path->prevArc(sta_);
        printf("repair_setup %s: %s ---> %s \n",
               prev_arc->from()->libertyCell()->name(),
               prev_arc->from()->name(),
               prev_arc->to()->name());
      }
    }
  }
  printf("done\n");
}

}  // namespace rsz
