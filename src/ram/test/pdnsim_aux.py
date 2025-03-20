############################################################################
##
## Copyright (c) 2023, The Regents of the University of Minnesota
## All rights reserved.
##
## BSD 3-Clause License
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
############################################################################
from openroad import Timing
import utl


def analyze_power_grid(
    design,
    *,
    vsrc=None,
    outfile=None,
    error_file=None,
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
        net, corner, 2, outfile, enable_em, em_outfile, error_file, vsrc
    )


def check_power_grid(design, *, net=None, error_file=None):
    pdnsim = design.getPDNSim()
    if not net:
        utl.error(utl.PSM, 157, "Argument 'net' not specified to check_power_grid.")

    if not error_file:
        error_file = ""

    res = pdnsim.checkConnectivity(design.getBlock().findNet(net), False, error_file)
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
