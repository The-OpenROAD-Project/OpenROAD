// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BufferMove.hh"

#include <cmath>
#include <cstdio>
#include <string>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;

using utl::RSZ;

using sta::ArcDelay;
using sta::Instance;
using sta::InstancePinIterator;
using sta::LoadPinIndexMap;
using sta::NetConnectedPinIterator;
using sta::Path;
using sta::PathExpanded;
using sta::Pin;
using sta::Slack;
using sta::Slew;
using sta::TimingArc;
using sta::Vertex;

BufferMove::BufferMove(Resizer* resizer) : BaseMove(resizer)
{
}

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
  if (!resizer_->okToBufferNet(drvr_pin)) {
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
               "opt_moves",
               1,
               "ACCEPT buffer {} inserted {}",
               network_->pathName(drvr_pin),
               rebuffer_count);
    addMove(drvr_inst, rebuffer_count);
  } else {
    debugPrint(logger_,
               RSZ,
               "opt_moves",
               3,
               "REJECT buffer {} inserted {}",
               network_->pathName(drvr_pin),
               rebuffer_count);
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
      const Vertex* path_vertex = path->vertex(sta_);
      const Pin* path_pin = path->pin(sta_);
      if (i > 0 && path_vertex->isDriver(network_)
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
