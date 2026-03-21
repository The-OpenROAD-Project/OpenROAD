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


def swig_list(vec):
    """Convert SWIG vector to Python list (avoids SWIG iterator bugs)."""
    return [vec[i] for i in range(len(vec))]


def approx(a, b, tol=1e-4):
    return abs(a - b) < tol


def pin_name(iterm, bterm):
    if iterm:
        return f"{iterm.getInst().getName()}/{iterm.getName()}"
    return bterm.getName()


# ── Summary metrics ─────────────────────────────────────────
wns = timing.getWorstSlack(Timing.Max)
tns = timing.getTotalNegativeSlack(Timing.Max)
count = timing.getEndpointCount()
assert count > 0

# ── Endpoint slacks: worst must equal WNS ───────────────────
slack_map = swig_list(timing.getEndpointSlacks(Timing.Max))
assert len(slack_map) == count
worst_ep_slack = min(e.slack for e in slack_map)
assert approx(
    worst_ep_slack, wns
), f"worst endpoint slack {worst_ep_slack} != WNS {wns}"

# Endpoints are register data pins — master must be sequential
for entry in slack_map:
    if entry.iterm:
        master = entry.iterm.getInst().getMaster()
        assert design.isSequential(
            master
        ), f"endpoint master {master.getName()} not sequential"

# ── Clock info: sources must connect to the design ──────────
clocks = swig_list(timing.getClockInfo())
assert len(clocks) > 0
for clk in clocks:
    assert clk.period > 0, f"clock {clk.name}: non-positive period"
    assert len(clk.waveform) >= 2, "waveform needs rise+fall edges"
    for src in swig_list(clk.source_bterms):
        net = src.getNet()
        assert net is not None, f"clock source {src.getName()} is unconnected"
        assert net.getITermCount() > 0, f"clock net {net.getName()} has no sinks"

# ── Timing paths: structural invariants ─────────────────────
# Called AFTER summary/endpoint/clock APIs — this call order is
# the regression test for the searchPreamble crash fix.
paths = swig_list(timing.getTimingPaths(Timing.Max, 5))
assert len(paths) > 0

for pi, path in enumerate(paths):
    arcs = swig_list(path.arcs)
    assert len(arcs) > 0

    # slack = required - arrival (fundamental STA identity)
    assert approx(path.slack, path.required - path.arrival), (
        f"path[{pi}]: slack {path.slack} != "
        f"required {path.required} - arrival {path.arrival}"
    )

    # Arc delays must sum to arrival time
    delay_sum = sum(a.delay for a in arcs)
    assert approx(delay_sum, path.arrival), (
        f"path[{pi}]: delay sum {delay_sum} != " f"arrival {path.arrival}"
    )

    # logic_depth counts unique instances, must be <= cell arcs
    cell_arcs = [a for a in arcs if a.master is not None]
    assert path.logic_depth <= len(cell_arcs)

    # Start/endpoint must match first/last arc pins
    assert pin_name(path.start_iterm, path.start_bterm) == pin_name(
        arcs[0].from_iterm, arcs[0].from_bterm
    )
    assert pin_name(path.end_iterm, path.end_bterm) == pin_name(
        arcs[-1].to_iterm, arcs[-1].to_bterm
    )

    for a in arcs:
        if a.master is None:
            # Net arc: pins on different instances (or port)
            if a.from_iterm and a.to_iterm:
                assert (
                    a.from_iterm.getInst().getName() != a.to_iterm.getInst().getName()
                ), "net arc endpoints on same instance"
        else:
            # Cell arc: pins on same instance, master consistent
            if a.from_iterm and a.to_iterm:
                assert (
                    a.from_iterm.getInst().getName() == a.to_iterm.getInst().getName()
                ), "cell arc pins on different instances"
                assert a.master.getName() == (
                    a.to_iterm.getInst().getMaster().getName()
                ), "arc master != instance master"

# ── Object cross-references on worst path ───────────────────
arcs = swig_list(paths[0].arcs)
cell_arcs = [a for a in arcs if a.master is not None]
net_arcs = [a for a in arcs if a.master is None]

# Cell-type classification (GUI timing display use case)
buf_count = sum(1 for a in cell_arcs if design.isBuffer(a.master))
inv_count = sum(1 for a in cell_arcs if design.isInverter(a.master))
assert buf_count + inv_count <= len(cell_arcs)

# Net fanout: arc fanout <= net iterm count (resizer use case)
for a in net_arcs:
    from_pin = a.from_iterm or a.from_bterm
    if from_pin:
        net = from_pin.getNet()
        if net:
            assert net.getITermCount() >= a.fanout, (
                f"net {net.getName()}: iterms "
                f"{net.getITermCount()} < fanout {a.fanout}"
            )

print("timing_report_api: PASS", flush=True)
sys.exit(0)
