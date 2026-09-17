// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <unordered_set>

namespace sta { class Net; class Pin; }

namespace rsz {

class Resizer;

// Serial, thread-unsafe ODB. Rank violating driver pins by |slack|*fanout,
// then call existing Rebuffer (rebufferPin via public rebufferNet). Cap work;
// do not buffer the whole design.
struct GpuBufferPassStats
{
  int candidates = 0;
  int nets_buffered = 0;
  int buffers_inserted = 0;
  double sta_ms = 0.0;
  double rank_ms = 0.0;
  double rebuffer_ms = 0.0;
  double post_ms = 0.0;
};

GpuBufferPassStats runGpuBufferPass(Resizer* resizer,
                                    int max_nets = 32,
                                    float wns_s = 0.0f,
                                    std::unordered_set<const sta::Pin*>* failed_pins = nullptr);

}  // namespace rsz
