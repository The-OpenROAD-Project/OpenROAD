# Output failures must roll back placement, clock connectivity and timing.
import contextlib
import os
from pathlib import Path
import resource
import signal
import sys
import tempfile

from openroad import Tech, Design
import wmk


@contextlib.contextmanager
def reject_file_writes():
    # A small claims file remains buffered until close. RLIMIT_FSIZE makes
    # that close fail on a real regular file, without filling the filesystem.
    # Send tool logs to a device so the limit only affects the claims output.
    sys.stdout.flush()
    sys.stderr.flush()
    saved = [os.dup(fd) for fd in (1, 2)]
    limit = resource.getrlimit(resource.RLIMIT_FSIZE)
    handler = signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
    try:
        with open(os.devnull, "w") as null:
            for fd in (1, 2):
                os.dup2(null.fileno(), fd)
        resource.setrlimit(resource.RLIMIT_FSIZE, (0, limit[1]))
        yield
    finally:
        resource.setrlimit(resource.RLIMIT_FSIZE, limit)
        signal.signal(signal.SIGXFSZ, handler)
        for fd, original in zip((1, 2), saved):
            os.dup2(original, fd)
            os.close(original)


def tcl(design, script):
    # evalTclString itself does not raise on Tcl errors.
    result = design.evalTclString(
        "if {[catch {" + script + "} result]} {return ERROR:$result}; set result"
    )
    assert not result.startswith("ERROR:"), result
    return result


def snapshot(design):
    return [
        (
            inst.getName(),
            inst.getLocation(),
            str(inst.getOrient()),
            str(inst.getPlacementStatus()),
            [
                (
                    pin.getMTerm().getName(),
                    pin.getNet().getName() if pin.getNet() else None,
                )
                for pin in inst.getITerms()
            ],
        )
        for inst in design.getBlock().getInsts()
    ]


def slacks(design):
    values = tcl(
        design,
        """
        set result {}
        foreach pin [get_pins -hierarchical *] {
            foreach prop {slack_max_rise slack_max_fall slack_min_rise slack_min_fall} {
                set slack [get_property $pin $prop]
                if {abs($slack) < 1e20} {
                    lappend result "[get_full_name $pin]:$prop=$slack"
                }
            }
        }
        join $result "|"
    """,
    )
    return {
        name: float(value)
        for name, value in (line.rsplit("=", 1) for line in values.split("|"))
    }


def check_timing(design, before):
    immediate = slacks(design)
    tcl(design, "estimate_parasitics -placement")
    fresh = slacks(design)
    assert before and before.keys() == immediate.keys() == fresh.keys()
    for name, value in before.items():
        assert abs(value - immediate[name]) < 1e-6, (
            name,
            value,
            immediate[name],
            fresh[name],
        )
        assert abs(value - fresh[name]) < 1e-6, name


def exercise(stage, directory):
    tech = Tech()
    tech.readLef("Nangate45/Nangate45.lef")
    tech.readLiberty("Nangate45/Nangate45_typ.lib")
    design = Design(tech)
    design.readDef("gcd_placed.def")
    tcl(
        design,
        """
        create_clock -name core_clock -period 2 [get_ports clk]
        set_wire_rc -signal -layer metal3
        set_wire_rc -clock -layer metal5
    """,
    )
    if stage == "cts":
        tcl(
            design,
            """
            clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 \
                -sink_clustering_enable
            set_propagated_clock [all_clocks]
        """,
        )
    tcl(design, "estimate_parasitics -placement")
    watermark = design.getWatermark()
    key = bytes(31) + bytes([4]) if stage == "cts" else bytes(32)
    if stage == "place":
        options = wmk.PlacementOptions()
        options.hpwl_eps_um = 1
        options.pair_dist_um = 3
        options.slack_threshold_ns = 0
        options.post_guard = False
        embed = lambda path: watermark.placementWatermark(key, options, str(path))
    else:
        options = wmk.CtsOptions()
        embed = lambda path: watermark.ctsWatermark(key, options, str(path))

    before, timing = snapshot(design), slacks(design)
    claims = directory / (stage + ".csv")
    old = b"previous claims must survive a failed replacement\n"
    claims.write_bytes(old)

    # Preflight rejects nonexistent parents and nonregular destinations.
    for invalid in (directory / "missing" / "claims", directory):
        try:
            embed(invalid)
        except RuntimeError as error:
            assert "Cannot prepare claims" in str(error), error
        else:
            raise AssertionError("invalid output path was accepted")
        assert snapshot(design) == before

    # Both the C++/Python API and the public Tcl command must report close-time
    # failures after embedding, and restore the complete pre-command state.
    for interface in ("python", "tcl"):
        with reject_file_writes():
            if interface == "python":
                try:
                    embed(claims)
                except RuntimeError as error:
                    assert "Cannot publish claims" in str(error), error
                else:
                    raise AssertionError("failed claim output reported success")
            else:
                args = (
                    "-hpwl_eps_um 1 -pair_dist_um 3 -slack_threshold_ns 0 -guard_degrade_ns 0"
                    if stage == "place"
                    else ""
                )
                result = design.evalTclString(
                    "set failed [catch {"
                    + stage
                    + "_watermark -key_hex "
                    + key.hex()
                    + " -claims_file {"
                    + str(claims)
                    + "} "
                    + args
                    + "} message]; list $failed [string match {*Cannot publish claims*} $message]"
                )
                assert result == "1 1", result
        assert snapshot(design) == before, (stage, interface)
        assert claims.read_bytes() == old
        assert not list(directory.glob(".wmk-claims-*"))
        check_timing(design, timing)

    # Without the injected failure, the same inputs must actually edit the
    # design. CTS needs several accepted moves to exercise whole-call rollback.
    count = embed(claims)
    after = snapshot(design)
    changes = sum(a != b for a, b in zip(before, after))
    assert count > 0 and changes >= 2, (stage, count, changes)
    assert claims.read_bytes() != old
    assert not list(directory.glob(".wmk-claims-*"))
    result = (
        watermark.verifyPlacement(str(claims))
        if stage == "place"
        else watermark.verifyCts(str(claims))
    )
    assert result.checked == count and result.held == count
    check_timing(design, slacks(design))
    print(stage, "publication and rollback pass; changed instances:", changes)


with tempfile.TemporaryDirectory(prefix="wmk-output-test-") as temporary:
    for stage in ("place", "cts"):
        exercise(stage, Path(temporary))
print("pass")
