# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

import os
import sys

from openroad import Tech, Design, Timing

# Handle Bazel runfiles working directory
test_dir = os.environ.get("TEST_SRCDIR", "")
if test_dir:
    os.chdir(os.path.join(test_dir, "_main/test"))

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")
design.evalTclString("read_sdc timing_api_3.sdc")
timing = Timing(design)

# Test getWorstSlack (setup)
setup_wns = timing.getWorstSlack(Timing.Max)
print(f"setup_wns={setup_wns:.6e}", flush=True)

# Test getTotalNegativeSlack (setup)
setup_tns = timing.getTotalNegativeSlack(Timing.Max)
print(f"setup_tns={setup_tns:.6e}", flush=True)

# Test getEndpointCount
count = timing.getEndpointCount()
assert count > 0, "should have endpoints"
print(f"endpoint_count={count}", flush=True)

# Test getEndpointSlackMap
slack_map = timing.getEndpointSlackMap(Timing.Max)
assert len(slack_map) == count, "slack map should match endpoint count"
print(f"slack_map_size={len(slack_map)}", flush=True)

# Test getClockInfo — use index access to avoid SWIG iterator bug
clocks = timing.getClockInfo()
assert len(clocks) > 0, "should have clocks"
for i in range(len(clocks)):
    clk = clocks[i]
    waveform = [clk.waveform[j] for j in range(len(clk.waveform))]
    sources = [clk.sources[j] for j in range(len(clk.sources))]
    print(
        f"clock: {clk.name} period={clk.period:.3f} "
        f"waveform={waveform} sources_len={len(sources)}",
        flush=True,
    )

# Test getTimingPaths (setup) — exercises searchPreamble fix.
# Called AFTER getWorstSlack/getTNS/getEndpointSlackMap/getClockInfo,
# which is the sequence that previously crashed.
paths = timing.getTimingPaths(Timing.Max, 5)
assert len(paths) > 0, "should have timing paths"
print(f"paths_returned={len(paths)}", flush=True)
for i in range(len(paths)):
    path = paths[i]
    print(
        f"path[{i}]: slack={path.slack:.4f} "
        f"start={path.startpoint[:30]} "
        f"end={path.endpoint[:30]} "
        f"arcs={len(path.arcs)} "
        f"depth={path.logic_depth}",
        flush=True,
    )

# Test arc detail structure
if len(paths) > 0 and len(paths[0].arcs) > 0:
    arc = paths[0].arcs[0]
    assert isinstance(arc.from_pin, str)
    assert isinstance(arc.to_pin, str)
    assert isinstance(arc.delay, float)
    assert isinstance(arc.slew, float)
    assert isinstance(arc.is_net, bool)
    print("arc_structure: OK", flush=True)

print("timing_report_api: PASS", flush=True)
sys.exit(0)
