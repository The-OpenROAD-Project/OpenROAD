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
from os.path import isfile


def analyze_power_grid(design, *,
                       vsrc=None,
                       outfile=None,
                       error_file=None,
                       enable_em=False,
                       em_outfile=None,
                       net=None,
                       dx=None,
                       dy=None,
                       node_density=None,
                       node_density_factor=None,
                       corner=None):
    pdnsim = design.getPDNSim()
    if bool(vsrc):
        if isfile(vsrc):
            pdnsim.setVsrcCfg(vsrc)
        else:
            utl.error(utl.PSM, 153, f"Cannot read {vsrc}.")

    if bool(net):
        pdnsim.setNet(design.getBlock().findNet(net))
    else:
        utl.error(utl.PSM, 154, "Argument 'net' not specified.")

    if bool(dx):
        pdnsim.setBumpPitchX(dx)

    if bool(dy):
        pdnsim.setBumpPitchY(dy)

    if bool(node_density) and bool(node_density_factor):
        utl.error(utl.PSM, 177, "Cannot use both node_density and " +
                  "node_density_factor together. Use any one argument")

    if bool(node_density):
       pdnsim.setNodeDensity(node_density)

    if bool(node_density_factor):
        pdnsim.setNodeDensityFactor(node_density_factor)

    if not outfile:
        outfile = ""

    if not error_file:
        error_file = ""

    _set_corner(design, corner)

    if bool(em_outfile):
        if not enable_em:
            utl.error(utl.PSM, 155, "EM outfile defined without EM " +
                      "enable flag. Add -enable_em.")
    else:
        em_outfile = ""

    if len(design.getBlock().getRows()) > 0:  # db_has_rows
        pdnsim.analyzePowerGrid(outfile, enable_em, em_outfile, error_file)
    else:
        utl.error(utl.PSM, 156, "No rows defined in design. "+
                  "Floorplan not defined. Use initialize_floorplan to add rows.");


def check_power_grid(design, *, net=None, error_file=None):
    pdnsim = design.getPDNSim()
    if bool(net):
        pdnsim.setNet(design.getBlock().findNet(net))
    else:
     utl.error(utl.PSM, 157, "Argument 'net' not specified to check_power_grid.")

    if not error_file:
        error_file = ""

    if len(design.getBlock().getRows()) > 0:  # db_has_rows
        res = pdnsim.checkConnectivity(error_file)
        if res == 0:
            utl.error(utl.PSM, 169, "Check connectivity failed.")
        return res
    else:
        utl.error(utl.PSM, 158, "No rows defined in design. " +
                   "Use initialize_floorplan to add rows.")


def write_pg_spice(design, *,
                   vsrc=None,
                   outfile=None,
                   net=None,
                   dx=None,
                   dy=None,
                   corner=None):
    pdnsim = design.getPDNSim()

    if bool(vsrc):
        if isfile(vsrc):
            pdnsim.setVsrcCfg(vsrc)
        else:
            utl.error(utl.PSM, 159, "Cannot read $vsrc_file.")

    if bool(net):
        pdnsim.setNet(design.getBlock().findNet(net))
    else:
        utl.error(utl.PSM, 160, "Argument 'net' not specified.")

    if bool(dx):
        pdnsim.setBumpPitchX(dx)

    if bool(dy):
        pdnsim.setBumpPitchY(dy)

    if not outfile:
        outfile = ""

    _set_corner(design, corner)

    if len(design.getBlock().getRows()) > 0:  # db_has_rows
        pdnsim.writeSpice(outfile)
    else:
        utl.error(utl.PSM, 161, "No rows defined in design. " +
                  "Use initialize_floorplan to add rows and construct PDN.")


def set_pdnsim_net_voltage(design, *, net=None, voltage=None):
    pdnsim = design.getPDNSim()
    if  bool(net) and bool(voltage):
        pdnsim.setNetVoltage(design.getBlock().findNet(net), float(voltage))
    else:
        utl.error(utl.PSM, 162, "Argument -net or -voltage not specified. " +
                  "Please specify both -net and -voltage arguments.")


def _set_corner(design, corner):
    if corner:
        design.evalTclString(f"psm::set_corner [sta::find_corner {corner}]")
    else:
        design.evalTclString(f"psm::set_corner [sta::cmd_corner]")
