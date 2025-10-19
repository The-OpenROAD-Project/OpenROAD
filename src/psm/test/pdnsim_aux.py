# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

from openroad import Timing
import utl


def analyze_power_grid(
    design,
    *,
    vsrc=None,
    outfile=None,
    error_file=None,
    allow_reuse=False,
    enable_em=False,
    em_outfile=None,
    net=None,
    corner=None
):
    pdnsim = design.getPDNSim()

    if not net:
        utl.error(utl.PSM, 154, "Argument 'net' not specified.")
    else:
        net = design.getBlock().findNet(net)

    if not outfile:
        outfile = ""

    if not error_file:
        error_file = ""

    if not vsrc:
        vsrc = ""

    corner = _find_corner(design, corner)

    if bool(em_outfile):
        if not enable_em:
            utl.error(
                utl.PSM,
                155,
                "EM outfile defined without EM " + "enable flag. Add -enable_em.",
            )
    else:
        em_outfile = ""

    pdnsim.analyzePowerGrid(
        net, corner, 2, outfile, allow_reuse, enable_em, em_outfile, error_file, vsrc
    )


def check_power_grid(design, *, net=None, error_file=None, require_bterm=True):
    pdnsim = design.getPDNSim()
    if not net:
        utl.error(utl.PSM, 157, "Argument 'net' not specified to check_power_grid.")

    if not error_file:
        error_file = ""

    res = pdnsim.checkConnectivity(
        design.getBlock().findNet(net), False, error_file, require_bterm
    )
    if res == 0:
        utl.error(utl.PSM, 169, "Check connectivity failed.")
    return res


def write_pg_spice(design, *, vsrc=None, outfile=None, net=None, corner=None):
    pdnsim = design.getPDNSim()

    if not net:
        utl.error(utl.PSM, 160, "Argument 'net' not specified.")

    if not outfile:
        outfile = ""

    if not vsrc:
        vsrc = ""

    corner = _find_corner(design, corner)

    pdnsim.writeSpiceNetwork(design.getBlock().findNet(net), corner, 2, outfile, vsrc)


def set_pdnsim_net_voltage(design, *, net=None, voltage=None, corner=None):
    pdnsim = design.getPDNSim()
    if bool(net) and bool(voltage):
        pdnsim.setNetVoltage(design.getBlock().findNet(net), corner, float(voltage))
    else:
        utl.error(
            utl.PSM,
            162,
            "Argument -net or -voltage not specified. "
            + "Please specify both -net and -voltage arguments.",
        )


def _find_corner(design, corner):
    timing = Timing(design)

    if not corner:
        return timing.cmdCorner()

    return timing.findCorner(corner)
