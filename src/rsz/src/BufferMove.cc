// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BufferMove.hh"

#include <cmath>
#include <cstdio>
#include <string>

#include "BaseMove.hh"
#include "rsz/Resizer.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace rsz {

using std::string;

using utl::RSZ;

BufferMove::BufferMove(Resizer* resizer) : BaseMove(resizer)
{
}

bool BufferMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  const sta::Pin* drvr_pin = drvr_path->pin(sta_);
  sta::Vertex* drvr_vertex = graph_->pinDrvrVertex(drvr_pin);
  sta::Instance* drvr = network_->instance(drvr_pin);

  const int fanout = this->fanout(drvr_vertex);
  if (fanout <= 1) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Fanout {} <= 1 min fanout",
               network_->pathName(drvr_pin),
               fanout);
    return false;
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
    return false;
  }

  if (!resizer_->okToBufferNet(drvr_pin)) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Not OK to buffer net",
               network_->pathName(drvr_pin));
    return false;
  }

  const int rebuffer_count = rebuffer(drvr_pin);

  if (rebuffer_count == 0) {
    debugPrint(logger_,
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Couldn't insert any buffers",
               network_->pathName(drvr_pin));
    return false;
  }

  debugPrint(logger_,
             RSZ,
             "buffer_move",
             1,
             "ACCEPT BufferMove {}: Inserted {} buffers",
             network_->pathName(drvr_pin),
             rebuffer_count);
  countMove(drvr, rebuffer_count);
  return true;
}

void BufferMove::debugCheckMultipleBuffers(sta::Path* path,
                                           sta::PathExpanded* expanded)
{
  if (expanded->size() > 1) {
    const int path_length = expanded->size();
    const int start_index = expanded->startIndex();
    for (int i = start_index; i < path_length; i++) {
      const sta::Path* path = expanded->path(i);
      const sta::Vertex* path_vertex = path->vertex(sta_);
      const sta::Pin* path_pin = path->pin(sta_);
      if (i > 0 && path_vertex->isDriver(network_)
          && !network_->isTopLevelPort(path_pin)) {
        const sta::TimingArc* prev_arc = path->prevArc(sta_);
        printf("repair_setup %s: %s ---> %s \n",
               prev_arc->from()->libertyCell()->name().c_str(),
               prev_arc->from()->name().c_str(),
               prev_arc->to()->name().c_str());
      }
    }
  }
  printf("done\n");
}

}  // namespace rsz
