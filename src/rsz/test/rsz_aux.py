# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

import os

import utl


def _warn(tool, msg_id, msg):
    if os.environ.get("TEST_SRCDIR", ""):
        print(f"Warning: {msg}")
    else:
        utl.warn(tool, msg_id, msg)


def _error(tool, msg_id, msg):
    if os.environ.get("TEST_SRCDIR", ""):
        raise ValueError(msg)
    else:
        utl.error(tool, msg_id, msg)


def buffer_ports(
    design,
    *,
    inputs=True,
    outputs=True,
    buffer_cell=None,
    max_utilization=None,
    verbose=False,
):
    resizer = design.getResizer()
    _set_max_utilization(resizer, max_utilization)
    if inputs:
        resizer.bufferInputs(buffer_cell, verbose)
    if outputs:
        resizer.bufferOutputs(buffer_cell, verbose)


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
    resizer = design.getResizer()
    _set_max_utilization(resizer, max_utilization)
    max_length_m = _parse_max_wire_length(max_wire_length)
    resizer.repairDesign(
        max_length_m,
        float(slew_margin),
        float(cap_margin),
        1.0 if pre_placement else 0.0,
        match_cell_footprint,
        verbose,
    )


def repair_clock_nets(design, *, max_wire_length=None):
    max_length_m = _parse_max_wire_length(max_wire_length)
    design.getResizer().repairClkNets(max_length_m)


def repair_tie_fanout(design, lib_port, *, separation=0, verbose=False):
    if lib_port is None:
        _error(utl.RSZ, 1020, "lib_port argument is required.")
    separation_m = separation * 1e-6
    design.getResizer().repairTieFanout(lib_port, separation_m, verbose)


def repair_timing(
    design,
    *,
    setup=True,
    hold=True,
    setup_margin=0.0,
    hold_margin=0.0,
    allow_setup_violations=False,
    sequence="",
    phases="",
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
    if not setup and not hold and recover_power is None:
        _warn(
            utl.RSZ,
            1033,
            "repair_timing: neither setup nor hold selected; nothing to repair.",
        )
        return

    resizer = design.getResizer()
    _set_max_utilization(resizer, max_utilization)

    if not (0 <= repair_tns <= 100):
        _error(utl.RSZ, 1030, "repair_tns must be between 0 and 100.")
    if not (0 <= max_buffer_percent <= 100):
        _error(utl.RSZ, 1031, "max_buffer_percent must be between 0 and 100.")

    if recover_power is not None:
        if not (0 <= recover_power <= 100):
            _error(utl.RSZ, 1032, "recover_power must be between 0 and 100.")
        resizer.recoverPower(recover_power / 100.0, match_cell_footprint, verbose)
        return

    repair_tns_fraction = repair_tns / 100.0
    max_buffer_fraction = max_buffer_percent / 100.0
    setup_margin_s = setup_margin * 1e-9
    hold_margin_s = hold_margin * 1e-9

    if setup:
        resizer.repairSetup(
            setup_margin_s,
            repair_tns_fraction,
            max_passes,
            max_iterations,
            max_repairs_per_pass,
            match_cell_footprint,
            verbose,
            sequence,
            phases,
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
        resizer.repairHold(
            setup_margin_s,
            hold_margin_s,
            allow_setup_violations,
            float(max_buffer_fraction),
            max_passes,
            max_iterations,
            match_cell_footprint,
            verbose,
        )


def report_design_area(design):
    resizer = design.getResizer()
    util = round(resizer.utilization() * 100)
    area = round(resizer.designArea() * 1e12)
    msg = f"Design area {area} um^2 {util}% utilization."
    # In Bazel use print as the global utl functions have been removed.
    if os.environ.get("TEST_SRCDIR", ""):
        print(msg)
    else:
        utl.report(msg)


def report_floating_nets(design):
    resizer = design.getResizer()
    floating_net_count = resizer.findFloatingNetsCount()
    floating_pin_count = resizer.findFloatingPinsCount()
    if floating_net_count > 0:
        _warn(utl.RSZ, 1040, f"found {floating_net_count} floating nets.")
    if floating_pin_count > 0:
        _warn(utl.RSZ, 1041, f"found {floating_pin_count} floating pins.")


def report_long_wires(design, count, *, digits=2):
    if not isinstance(count, int) or count < 0:
        _error(utl.RSZ, 1052, "report_long_wires count must be a non-negative integer.")
    design.getResizer().reportLongWires(count, digits)


def _set_max_utilization(resizer, max_utilization):
    if max_utilization is None:
        resizer.setMaxUtilization(0.0)
        return
    if not (0.0 <= max_utilization <= 100.0):
        _error(utl.RSZ, 1050, "max_utilization must be between 0 and 100.")
    resizer.setMaxUtilization(max_utilization / 100.0)


def _parse_max_wire_length(max_wire_length):
    if max_wire_length is None:
        return 0.0
    if not isinstance(max_wire_length, (int, float)) or max_wire_length <= 0:
        _error(utl.RSZ, 1051, "max_wire_length must be a positive number.")
    return max_wire_length * 1e-6
