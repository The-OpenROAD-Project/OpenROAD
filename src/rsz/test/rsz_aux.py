# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

import utl


def set_dont_use(design, lib_cells):
    resizer = design.getResizer()
    for cell in lib_cells:
        resizer.setDontUse(cell, True)


def unset_dont_use(design, lib_cells):
    resizer = design.getResizer()
    for cell in lib_cells:
        resizer.setDontUse(cell, False)


def reset_dont_use(design):
    design.getResizer().resetDontUse()


def set_dont_touch_instance(design, instances):
    resizer = design.getResizer()
    for inst in instances:
        resizer.setDontTouch(inst, True)


def unset_dont_touch_instance(design, instances):
    resizer = design.getResizer()
    for inst in instances:
        resizer.setDontTouch(inst, False)


def set_dont_touch_net(design, nets):
    resizer = design.getResizer()
    for net in nets:
        resizer.setDontTouch(net, True)


def unset_dont_touch_net(design, nets):
    resizer = design.getResizer()
    for net in nets:
        resizer.setDontTouch(net, False)


def report_dont_use(design):
    design.getResizer().reportDontUse()


def report_dont_touch(design):
    design.getResizer().reportDontTouch()


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


def remove_buffers(design):
    design.getResizer().removeBuffers()


def balance_row_usage(design):
    design.getResizer().balanceRowUsage()


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


def repair_clock_inverters(design):
    design.getResizer().repairClkInverters()


def repair_tie_fanout(design, lib_port, *, separation=0, verbose=False):
    if lib_port is None:
        utl.error(utl.RSZ, 1020, "lib_port argument is required.")
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
    resizer = design.getResizer()
    _set_max_utilization(resizer, max_utilization)

    if not (0 <= repair_tns <= 100):
        utl.error(utl.RSZ, 1030, "repair_tns must be between 0 and 100.")
    if not (0 <= max_buffer_percent <= 100):
        utl.error(utl.RSZ, 1031, "max_buffer_percent must be between 0 and 100.")

    if recover_power is not None:
        if not (0 <= recover_power <= 100):
            utl.error(utl.RSZ, 1032, "recover_power must be between 0 and 100.")
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
    utl.report(f"Design area {area} um^2 {util}% utilization.")


def report_floating_nets(design):
    resizer = design.getResizer()
    floating_net_count = resizer.findFloatingNetsCount()
    floating_pin_count = resizer.findFloatingPinsCount()
    if floating_net_count > 0:
        utl.warn(utl.RSZ, 1040, f"found {floating_net_count} floating nets.")
    if floating_pin_count > 0:
        utl.warn(utl.RSZ, 1041, f"found {floating_pin_count} floating pins.")


def report_overdriven_nets(design, *, include_parallel_driven=False):
    resizer = design.getResizer()
    overdriven_net_count = resizer.findOverdrivenNetsCount(include_parallel_driven)
    if overdriven_net_count > 0:
        utl.warn(utl.RSZ, 1042, f"found {overdriven_net_count} overdriven nets.")


def report_long_wires(design, count, *, digits=2):
    if not isinstance(count, int) or count < 0:
        utl.error(
            utl.RSZ, 1052, "report_long_wires count must be a non-negative integer."
        )
    design.getResizer().reportLongWires(count, digits)


def eliminate_dead_logic(design):
    design.getResizer().eliminateDeadLogic(True)


def _set_max_utilization(resizer, max_utilization):
    if max_utilization is None:
        resizer.setMaxUtilization(0.0)
        return
    if not (0.0 <= max_utilization <= 100.0):
        utl.error(utl.RSZ, 1050, "max_utilization must be between 0 and 100.")
    resizer.setMaxUtilization(max_utilization / 100.0)


def _parse_max_wire_length(max_wire_length):
    if max_wire_length is None:
        return 0.0
    if not isinstance(max_wire_length, (int, float)) or max_wire_length <= 0:
        utl.error(utl.RSZ, 1051, "max_wire_length must be a positive number.")
    return max_wire_length * 1e-6
