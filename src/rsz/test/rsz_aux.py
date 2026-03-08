# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

import utl
import rsz


# ---------------------------------------------------------------------------
# Don't-use / Don't-touch
# ---------------------------------------------------------------------------


def set_dont_use(design, lib_cells):
    """Mark library cells as not to be used by the resizer and CTS engines.

    keyword arguments:
    lib_cells -- list of LibertyCell objects returned by get_lib_cells
    """
    if not lib_cells:
        utl.error(utl.RSZ, 1000, "lib_cells argument is required.")
    for cell in lib_cells:
        rsz.set_dont_use(cell, True)


def unset_dont_use(design, lib_cells):
    """Reverse the effect of set_dont_use for the given library cells.

    keyword arguments:
    lib_cells -- list of LibertyCell objects
    """
    if not lib_cells:
        utl.error(utl.RSZ, 1001, "lib_cells argument is required.")
    for cell in lib_cells:
        rsz.set_dont_use(cell, False)


def reset_dont_use(design):
    """Restore the default dont-use list (undo all set_dont_use calls)."""
    rsz.reset_dont_use()


def set_dont_touch_instance(design, instances):
    """Prevent the resizer from modifying the given instances.

    keyword arguments:
    instances -- list of Instance objects
    """
    if not instances:
        utl.error(utl.RSZ, 1002, "instances argument is required.")
    for inst in instances:
        rsz.set_dont_touch_instance(inst, True)


def unset_dont_touch_instance(design, instances):
    """Reverse the effect of set_dont_touch_instance for the given instances.

    keyword arguments:
    instances -- list of Instance objects
    """
    if not instances:
        utl.error(utl.RSZ, 1003, "instances argument is required.")
    for inst in instances:
        rsz.set_dont_touch_instance(inst, False)


def set_dont_touch_net(design, nets):
    """Prevent the resizer from modifying the given nets.

    keyword arguments:
    nets -- list of Net objects
    """
    if not nets:
        utl.error(utl.RSZ, 1004, "nets argument is required.")
    for net in nets:
        rsz.set_dont_touch_net(net, True)


def unset_dont_touch_net(design, nets):
    """Reverse the effect of set_dont_touch_net for the given nets.

    keyword arguments:
    nets -- list of Net objects
    """
    if not nets:
        utl.error(utl.RSZ, 1005, "nets argument is required.")
    for net in nets:
        rsz.set_dont_touch_net(net, False)


def report_dont_use(design):
    """Report all library cells marked as dont-use."""
    rsz.report_dont_use()


def report_dont_touch(design):
    """Report all instances and nets marked as dont-touch."""
    rsz.report_dont_touch()


# ---------------------------------------------------------------------------
# Port buffering
# ---------------------------------------------------------------------------


def buffer_ports(
    design,
    *,
    inputs=True,
    outputs=True,
    buffer_cell=None,
    max_utilization=None,
    verbose=False,
):
    """Insert buffers on primary input and/or output ports.

    Inserting buffers on ports makes the block's input capacitances and output
    drives independent of its internals.

    keyword arguments:
    inputs          -- buffer primary inputs (default True)
    outputs         -- buffer primary outputs (default True)
    buffer_cell     -- LibertyCell to use for buffering; None lets the resizer
                       choose automatically
    max_utilization -- cap on the percentage of core area used (0-100)
    verbose         -- enable verbose logging (default False)
    """
    _set_max_utilization(max_utilization)

    if inputs:
        rsz.buffer_inputs(buffer_cell, verbose)
    if outputs:
        rsz.buffer_outputs(buffer_cell, verbose)


def remove_buffers(design):
    """Remove buffers inserted by synthesis.

    Running this before repair_design gives the resizer maximum flexibility
    when re-buffering nets.  Direct input-to-output feedthrough buffers, and
    buffers that are dont-touch or fixed-cell, are never removed.
    """
    rsz.remove_buffers_cmd()


def balance_row_usage(design):
    """Balance row usage across the placement rows."""
    rsz.balance_row_usage_cmd()


# ---------------------------------------------------------------------------
# Design repair
# ---------------------------------------------------------------------------


def repair_design(
    design,
    *,
    max_wire_length=None,
    slew_margin=0,
    cap_margin=0,
    max_utilization=None,
    pre_placement=False,
    match_cell_footprint=False,
    verbose=False,
):
    """Insert buffers and resize gates to fix max-slew, max-cap, max-fanout,
    and long-wire violations.

    Use estimate_parasitics -placement before calling this command.

    keyword arguments:
    max_wire_length      -- maximum wire length in microns; defaults to the
                            value that minimises wire delay for the wire RC set
                            by set_wire_rc
    slew_margin          -- additional slew margin as a percentage [0, 100)
    cap_margin           -- additional capacitance margin as a percentage
                            [0, 100)
    max_utilization      -- cap on the percentage of core area used (0-100)
    pre_placement        -- perform an initial pre-placement sizing/buffering
                            round (default False)
    match_cell_footprint -- obey Liberty cell footprint when swapping gates
    verbose              -- enable verbose logging (default False)
    """
    _set_max_utilization(max_utilization)

    max_length_m = _parse_max_wire_length(max_wire_length)

    if not (0 <= slew_margin < 100):
        utl.warn(utl.RSZ, 1010, "-slew_margin must be between 0 and 100 percent.")
    if not (0 <= cap_margin < 100):
        utl.warn(utl.RSZ, 1011, "-cap_margin must be between 0 and 100 percent.")

    rsz.repair_design_cmd(
        max_length_m,
        float(slew_margin),
        float(cap_margin),
        pre_placement,
        match_cell_footprint,
        verbose,
    )


def repair_clock_nets(design, *, max_wire_length=None):
    """Insert buffers on the wire from the clock input pin to the clock root
    buffer to reduce long-wire delays introduced by clock_tree_synthesis.

    keyword arguments:
    max_wire_length -- maximum wire length in microns; defaults to the value
                       that minimises wire delay for the wire RC set by
                       set_wire_rc
    """
    max_length_m = _parse_max_wire_length(max_wire_length)
    rsz.repair_clk_nets_cmd(max_length_m)


def repair_clock_inverters(design):
    """Replace each inverter in the clock tree that fans out to multiple sinks
    with one inverter per sink, so that CTS sees an undivided clock tree.

    Run this before clock_tree_synthesis.
    """
    rsz.repair_clk_inverters_cmd()


def repair_tie_fanout(design, lib_port, *, separation=0, verbose=False):
    """Connect each tie-high/low load to its own copy of the tie cell.

    keyword arguments:
    lib_port   -- LibertyPort for the tie-high or tie-low output pin
    separation -- distance between the tie cell and its load in microns
                  (default 0)
    verbose    -- enable verbose logging (default False)
    """
    if lib_port is None:
        utl.error(utl.RSZ, 1020, "lib_port argument is required.")
    if separation < 0:
        utl.error(utl.RSZ, 1021, "-separation must be non-negative.")

    separation_m = separation * 1e-6
    rsz.repair_tie_fanout_cmd(lib_port, separation_m, verbose)


# ---------------------------------------------------------------------------
# Timing repair
# ---------------------------------------------------------------------------


def repair_timing(
    design,
    *,
    setup=True,
    hold=True,
    setup_margin=0.0,
    hold_margin=0.0,
    allow_setup_violations=False,
    sequence="",
    skip_pin_swap=False,
    skip_gate_cloning=False,
    skip_size_down=False,
    skip_buffering=False,
    skip_buffer_removal=False,
    skip_last_gasp=False,
    skip_vt_swap=False,
    skip_crit_vt_swap=False,
    repair_tns=100,
    max_passes=10000,
    max_iterations=-1,
    max_repairs_per_pass=1,
    max_buffer_percent=20,
    max_utilization=None,
    recover_power=None,
    match_cell_footprint=False,
    verbose=False,
):
    """Repair setup and/or hold timing violations.

    Run after clock tree synthesis with propagated clocks.  Setup repair is
    performed before hold repair so that hold fixes do not create new setup
    violations.

    keyword arguments:
    setup                  -- repair setup timing (default True)
    hold                   -- repair hold timing (default True)
    setup_margin           -- extra setup slack margin in nanoseconds
    hold_margin            -- extra hold slack margin in nanoseconds
    allow_setup_violations -- permit buffers that worsen setup when fixing hold
    sequence               -- comma-separated order of setup optimisations;
                              default is
                              "unbuffer,vt_swap,sizeup,swap,buffer,clone,split"
    skip_pin_swap          -- skip pin-swap optimisation
    skip_gate_cloning      -- skip gate-cloning optimisation
    skip_size_down         -- skip non-critical gate down-sizing
    skip_buffering         -- skip rebuffering and load-splitting
    skip_buffer_removal    -- skip buffer-removal optimisation
    skip_last_gasp         -- skip final greedy sizing pass
    skip_vt_swap           -- skip threshold-voltage swap optimisation
    skip_crit_vt_swap      -- skip critical VT swap optimisation
    repair_tns             -- percentage of violating endpoints to repair
                              [0, 100] (default 100)
    max_passes             -- maximum optimisation passes (default 10000)
    max_iterations         -- maximum iterations; -1 disables the limit
    max_repairs_per_pass   -- maximum repairs attempted per pass (default 1)
    max_buffer_percent     -- maximum buffers to insert for hold fixing as a
                              percentage of the instance count [0, 100]
                              (default 20)
    max_utilization        -- cap on the percentage of core area used (0-100)
    recover_power          -- percentage of positive-slack paths to consider
                              for power recovery (0, 100]; when set,
                              setup/hold repair is skipped
    match_cell_footprint   -- obey Liberty cell footprint when swapping gates
    verbose                -- enable verbose logging (default False)
    """
    _set_max_utilization(max_utilization)

    if not (0 <= repair_tns <= 100):
        utl.error(utl.RSZ, 1030, "-repair_tns must be between 0 and 100.")
    if not (0 <= max_buffer_percent <= 100):
        utl.error(utl.RSZ, 1031, "-max_buffer_percent must be between 0 and 100.")

    if recover_power is not None:
        if not (0 <= recover_power <= 100):
            utl.error(utl.RSZ, 1032, "-recover_power must be between 0 and 100.")
        rsz.recover_power(recover_power / 100.0, match_cell_footprint, verbose)
        return

    repair_tns_fraction = repair_tns / 100.0
    max_buffer_fraction = max_buffer_percent / 100.0

    setup_margin_s = setup_margin * 1e-9
    hold_margin_s = hold_margin * 1e-9

    if setup:
        rsz.repair_setup(
            setup_margin_s,
            repair_tns_fraction,
            max_passes,
            max_iterations,
            max_repairs_per_pass,
            match_cell_footprint,
            verbose,
            sequence,
            skip_pin_swap,
            skip_gate_cloning,
            skip_size_down,
            skip_buffering,
            skip_buffer_removal,
            skip_last_gasp,
            skip_vt_swap,
            skip_crit_vt_swap,
        )

    if hold:
        rsz.repair_hold(
            setup_margin_s,
            hold_margin_s,
            allow_setup_violations,
            float(max_buffer_fraction),
            max_passes,
            max_iterations,
            match_cell_footprint,
            verbose,
        )


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def report_design_area(design):
    """Report the total area of the design's components and the utilisation."""
    util = round(rsz.utilization() * 100)
    area = round(rsz.design_area() * 1e12)
    utl.report(f"Design area {area} um^2 {util}% utilization.")


def report_floating_nets(design, *, verbose=False):
    """Report nets that have loads but no driver.

    keyword arguments:
    verbose -- reserved for future use; individual net names are not yet
               available via the Python API
    """
    floating_net_count = rsz.find_floating_nets_count()
    floating_pin_count = rsz.find_floating_pins_count()

    if floating_net_count > 0:
        utl.warn(utl.RSZ, 1040, f"found {floating_net_count} floating nets.")
    if floating_pin_count > 0:
        utl.warn(utl.RSZ, 1041, f"found {floating_pin_count} floating pins.")


def report_overdriven_nets(design, *, include_parallel_driven=False):
    """Report nets that are driven by more than one driver.

    keyword arguments:
    include_parallel_driven -- include nets with multiple parallel drivers
    """
    overdriven_net_count = rsz.find_overdriven_nets_count(include_parallel_driven)
    if overdriven_net_count > 0:
        utl.warn(utl.RSZ, 1042, f"found {overdriven_net_count} overdriven nets.")


def report_long_wires(design, count, *, digits=2):
    """Report the longest wires in the design.

    keyword arguments:
    count  -- number of long wires to report
    digits -- number of digits for floating-point values (default 2)
    """
    if not isinstance(count, int) or count < 0:
        utl.error(utl.RSZ, 1052, "report_long_wires count must be a non-negative integer.")
    rsz.report_long_wires_cmd(count, digits)


def eliminate_dead_logic(design):
    """Remove standard-cell instances that can be deleted without affecting
    the design's functional behaviour.
    """
    rsz.eliminate_dead_logic_cmd(True)


# ---------------------------------------------------------------------------
# Internal helpers (not part of the public API)
# ---------------------------------------------------------------------------


def _set_max_utilization(max_utilization):
    """Validate and set max-utilization percentage; None uses C++ default (0.0)."""
    if max_utilization is None:
        rsz.set_max_utilization(0.0)
        return
    if not (0.0 <= max_utilization <= 100.0):
        utl.error(utl.RSZ, 1050, "-max_utilization must be between 0 and 100.")
    rsz.set_max_utilization(max_utilization / 100.0)


def _parse_max_wire_length(max_wire_length):
    """Convert microns to metres; None returns 0.0 (C++ uses default)."""
    if max_wire_length is None:
        return 0.0
    if not isinstance(max_wire_length, (int, float)) or max_wire_length <= 0:
        utl.error(utl.RSZ, 1051, "-max_wire_length must be a positive number.")
    return max_wire_length * 1e-6
