#############################################################################
##
## Copyright (c) 2022, The Regents of the University of California
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
#############################################################################
import utl


def macro_placement(
    design, *, halo=None, channel=None, fence_region=None, snap_layer=None, style=None
):
    if halo != None:
        if len(halo) != 2:
            utl.error(
                utl.MPL, 192, f"halo receives a list with 2 values, {len(halo)} given."
            )
        halo_x, halo_y = halo
        if is_pos_float(halo_x) and is_pos_float(halo_y):
            design.getMacroPlacer().setHalo(halo_x, halo_y)

    if channel != None:
        if length(channel) != 2:
            utl.error(
                utl.MPL,
                193,
                f"channel receives a list with 2 values, {len(channel)} given.",
            )

        channel_x, channel_y = channel
        if is_pos_float(channel_x) and is_pos_float(channel_y):
            design.getMacroPlacer().setChannel(channel_x, channel_y)

    if len(design.getBlock().getRows()) < 1:
        utl.error(utl.MPL, 189, "No rows found. Use initialize_floorplan to add rows.")

    core = design.getBlock().getCoreArea()
    units = design.getBlock().getDbUnitsPerMicron()
    core_lx = core.xMin() / units
    core_ly = core.yMin() / units
    core_ux = core.xMax() / units
    core_uy = core.yMax() / units

    if fence_region != None:
        if len(fence_region) != 4:
            utl.error(
                utl.MPL,
                194,
                f"fence_region receives a list with 4 values, {len(fence_region)} given.",
            )

        lx, ly, ux, uy = fence_region

        if lx < core_lx or ly < core_ly or ux > core_ux or uy > core_uy:
            utl.warn(
                utl.MPL, 185, "fence_region outside of core area. Using core area."
            )
            design.getMacroPlacer().setFenceRegion(core_lx, core_ly, core_ux, core_uy)
        else:
            design.getMacroPlacer().setFenceRegion(lx, ly, ux, uy)
    else:
        design.getMacroPlacer().setFenceRegion(core_lx, core_ly, core_ux, core_uy)

    if not is_pos_int(snap_layer):
        snap_layer = 4

    rtech = design.getTech().getDB().getTech()
    layer = rtech.findRoutingLayer(snap_layer)

    if layer == None:
        utl.error(utl.MPL, 195, f"Snap layer {snap_layer} is not a routing layer.")

    design.getMacroPlacer().setSnapLayer(layer)

    if style == None:
        style = "corner_max_wl"

    if style == "corner_max_wl":
        design.getMacroPlacer().placeMacrosCornerMaxWl()
    elif style == "corner_min_wl":
        design.getMacroPlacer().placeMacrosCornerMinWL()
    else:
        utl.error(
            utl.MPL,
            196,
            "Unknown placement style. Use one of 'corner_max_wl' or 'corner_min_wl'.",
        )


def is_pos_int(x):
    if x == None:
        return False
    elif isinstance(x, int) and x > 0:
        return True
    else:
        utl.error(utl.GPL, 507, f"TypeError: {x} is not a positive integer")
    return False


def is_pos_float(x):
    if x == None:
        return False
    elif isinstance(x, float) and x >= 0:
        return True
    else:
        utl.error(utl.MPL, 202, f"TypeError: {x} is not a positive float")
    return False
