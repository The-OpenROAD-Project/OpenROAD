############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2022, The Regents of the University of California
## All rights reserved.
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
import openroad as ord
import odb


class IFPError(Exception):
    def __init__(self, msg):
        print(msg)


# To be removed once we have UPF support
def create_voltage_domain(design, domain_name, area):
    # which flavor of error reporting should be used here?
    if len(area) != 4:
        raise IFPError("utl::error ODB 315 '-area is a list of 4 coordinates'")

    block = design.getBlock()

    if block == None:
        raise IFPError(
            "utl::error ODB 317 'please load the design before trying to use this command'"
        )

    region = odb.dbRegion_create(block, domain_name)

    if region == None:
        raise IFPError("utl::error ODB 318 'duplicate region name'")

    lx, ly, ux, uy = area
    box = odb.dbBox_create(region, lx, ly, ux, uy)
    group = odb.dbGroup_create(region, domain_name)

    if group == None:
        raise IFPError("utl::error ODB 319 'duplicate group name'")

    group.setType("VOLTAGE_DOMAIN")


def insert_tiecells(tech, floorplan, args, prefix=None):
    tie_pin_split = args.split("/")
    port = tie_pin_split[-1]
    tie_cell = "/".join(tie_pin_split[0:-1])
    master = None

    db = tech.getDB()

    for lib in db.getLibs():
        master = lib.findMaster(tie_cell)
        if master != None:
            break

    if master == None:
        raise IFPError(f"IFP 31 Unable to find master: {tie_cell}")

    mterm = master.findMTerm(port)
    if mterm == None:
        raise IFPError(f"IFP 32 Unable to find master pin: {args}")

    if prefix:
        floorplan.insertTiecells(mterm, prefix)
    else:
        floorplan.insertTiecells(mterm)
