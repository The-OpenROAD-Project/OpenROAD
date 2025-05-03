// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "BufferMove.hh"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "CloneMove.hh"
#include "SizeMove.hh"
#include "SplitLoadMove.hh"
#include "SwapPinsMove.hh"

#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PathExpanded.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

using sta::PathExpanded;


bool BufferMove::doMove(const Path* drvr_path,
                        const int drvr_index,
                        PathExpanded* expanded)
{
    Vertex* drvr_vertex = drvr_path->vertex(sta_);
    const Pin* drvr_pin = drvr_vertex->pin();
    Instance* drvr_inst = network_->instance(drvr_pin);

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

