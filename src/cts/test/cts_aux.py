###############################################################################
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
##   and#or other materials provided with the distribution.
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
###############################################################################
from openroad import Design, Tech


def clock_tree_synthesis(
    design,
    *,
    wire_unit=None,
    buf_list=None,
    root_buf=None,
    clk_nets=None,
    tree_buf=None,
    distance_between_buffers=None,
    branching_point_buffers_distance=None,
    clustering_exponent=None,
    clustering_unbalance_ratio=None,
    sink_clustering_size=None,
    sink_clustering_max_diameter=None,
    sink_clustering_enable=False,
    balance_levels=False,
    sink_clustering_levels=None,
    num_static_layers=None,
    sink_clustering_buffer=None,
    obstruction_aware=False,
    apply_ndr=False,
):
    cts = design.getTritonCts()
    parms = cts.getParms()

    # Boolean
    parms.setSinkClustering(sink_clustering_enable)
    parms.setBalanceLevels(balance_levels)
    parms.setObstructionAware(obstruction_aware)
    parms.setApplyNDR(apply_ndr)

    if is_pos_int(sink_clustering_size):
        parms.setSinkClusteringSize(sink_clustering_size)

    if is_pos_float(sink_clustering_max_diameter):
        parms.setMaxDiameter(sink_clustering_max_diameter)

    if is_pos_int(sink_clustering_levels):
        parms.setSinkClusteringLevels(sink_clustering_levels)

    if is_pos_int(num_static_layers):
        parms.setNumStaticLayers(num_static_layers)

    if is_pos_float(distance_between_buffers):
        parms.setSimpleSegmentsEnabled(True)
        parms.setBufferDistance(design.micronToDBU(distance_between_buffers))

    if is_pos_float(branching_point_buffers_distance):
        parms.setVertexBuffersEnabled(True)
        parms.setVertexBufferDistance(
            design.micronToDBU(branching_point_buffers_distance)
        )

    if is_pos_int(clustering_exponent):
        parms.setClusteringPower(clustering_exponent)

    if is_pos_float(clustering_unbalance_ratio):
        parms.setClusteringCapacity(clustering_unbalance_ratio)

    # NOTE: buf_list is not really a list but a single string with
    # space separated buffer names
    if buf_list != None:
        cts.setBufferList(buf_list)
    else:
        cts.setBufferList("")

    if is_pos_int(wire_unit):
        parms.setWireSegmentUnit(wire_unit)

    if clk_nets != None:
        success = cts.setClockNets(clk_nets)
        if success != 0:
            utl.error(utl.CTS, 602, "Error when finding clk_nets in DB.")

    if tree_buf != None:
        parms.setTreeBuffer(tree_buf)

    if root_buf != None:
        cts.setRootBuffer(root_buf)
    else:
        cts.setRootBuffer("")

    if sink_clustering_buffer != None:
        cts.setSinkBuffer(sink_clustering_buf)
    else:
        cts.setSinkBuffer("")

    if design.getBlock() == None:
        utl.error(utl.CTS, 604, "No design block found.")

    cts.runTritonCts()


def report_cts(design, out_file=None):
    if out_file != None:
        design.getTritonCts.getParms().setMetricsFile(out_file)


def is_pos_int(x):
    if x == None:
        return False
    elif isinstance(x, int) and x > 0:
        return True
    else:
        utl.error(utl.CTS, 605, f"TypeError: {x} is not a postive integer")
    return False


def is_pos_float(x):
    if x == None:
        return False
    elif isinstance(x, float) and x > 0.0:
        return True
    else:
        utl.error(utl.CTS, 606, f"TypeError: {x} is not a positive float")
    return False
