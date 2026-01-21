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

bool BufferMove::doMove(const Pin* drvr_pin, float setup_slack_margin)
{
  startMove(drvr_pin);

  Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
  Instance* drvr = network_->instance(drvr_pin);

  const int fanout = this->fanout(drvr_vertex);
  if (fanout <= 1) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Fanout {} <= 1 min fanout",
               network_->pathName(drvr_pin),
               fanout);
    return endMove(false);
  }

  // Rebuffer blows up on large fanout nets.
  if (fanout >= rebuffer_max_fanout_) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Fanout {} >= {} max fanout limit",
               network_->pathName(drvr_pin),
               fanout,
               rebuffer_max_fanout_);
    return endMove(false);
  }

  if (!resizer_->okToBufferNet(drvr_pin)) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Not OK to buffer net",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  const int rebuffer_count = rebuffer(drvr_pin);

  if (rebuffer_count == 0) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Couldn't insert any buffers",
               network_->pathName(drvr_pin));
    return endMove(false);
  }

  debugPrint(logger_,
             RSZ,
             "buffer_move",
             1,
             "ACCEPT BufferMove {}: Inserted {} buffers",
             network_->pathName(drvr_pin),
             rebuffer_count);
  countMove(drvr, rebuffer_count);
  return endMove(true);
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
