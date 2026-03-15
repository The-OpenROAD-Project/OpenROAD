# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

from openroad import Tech, Design, Timing

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = Design(tech)
design.readDef("gcd_nangate45.def")
design.evalTclString("read_sdc timing_api_3.sdc")
timing = Timing(design)

# Test getWorstSlack
setup_wns = timing.getWorstSlack(Timing.Max)
hold_wns = timing.getWorstSlack(Timing.Min)
print(f"setup_wns_finite: {not timing.isTimeInf(setup_wns)}")
print(f"hold_wns_finite: {not timing.isTimeInf(hold_wns)}")

# Test getTotalNegativeSlack
setup_tns = timing.getTotalNegativeSlack(Timing.Max)
hold_tns = timing.getTotalNegativeSlack(Timing.Min)
print(f"setup_tns_type: {type(setup_tns).__name__}")
print(f"hold_tns_type: {type(hold_tns).__name__}")

# Test getEndpointCount
count = timing.getEndpointCount()
print(f"endpoint_count_positive: {count > 0}")

# Test getEndpointSlackMap
slack_map = timing.getEndpointSlackMap(Timing.Max)
print(f"slack_map_nonempty: {len(slack_map) > 0}")
print(f"slack_map_matches_count: {len(slack_map) == count}")
# Check that entries have string names and float slacks
first_name, first_slack = slack_map[0]
print(f"slack_map_name_type: {type(first_name).__name__}")
print(f"slack_map_slack_type: {type(first_slack).__name__}")

# Test getClockInfo
clocks = timing.getClockInfo()
print(f"clocks_nonempty: {len(clocks) > 0}")
for clk in clocks:
    print(f"clock: {clk.name} period={clk.period:.3f} "
          f"waveform_len={len(clk.waveform)} sources_len={len(clk.sources)}")

# Test getTimingPaths
paths = timing.getTimingPaths(Timing.Max, 5)
print(f"paths_returned: {len(paths)}")
for i, path in enumerate(paths):
    print(f"path[{i}]: slack={path.slack:.4f} "
          f"start={path.startpoint[:30]} "
          f"end={path.endpoint[:30]} "
          f"arcs={len(path.arcs)} "
          f"depth={path.logic_depth}")

# Test that arcs have expected structure
if len(paths) > 0 and len(paths[0].arcs) > 0:
    arc = paths[0].arcs[0]
    print(f"arc_has_from_pin: {len(arc.from_pin) > 0}")
    print(f"arc_has_to_pin: {len(arc.to_pin) > 0}")
    print(f"arc_delay_type: {type(arc.delay).__name__}")
    print(f"arc_slew_type: {type(arc.slew).__name__}")
    print(f"arc_is_net_type: {type(arc.is_net).__name__}")

# Test hold paths
hold_paths = timing.getTimingPaths(Timing.Min, 3)
print(f"hold_paths_returned: {len(hold_paths)}")

print("timing_report_api: PASS")
