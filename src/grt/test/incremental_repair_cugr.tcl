# Test CUGR incremental routing with repair_design + repair_timing
# transformations, mirroring the ORFS global_route.tcl flow.
#
# Flow:
#   1. Initial global_route -use_cugr
#   2. estimate_parasitics -global_routing
#   3. repair_design  (internally calls start/end_incremental)
#   4. detailed_placement to fix overlaps
#   5. global_route -start_incremental / -end_incremental for DPL changes
#   6. estimate_parasitics -global_routing
#   7. repair_timing -setup (internally calls start/end_incremental)
#   8. detailed_placement + incremental re-route for timing changes
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd.def

create_clock [get_ports clk] -name core_clock -period 2.0
set_max_fanout 100 [current_design]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# 1. Initial global route with CUGR
global_route -verbose -use_cugr

# 2. Repair design using global route parasitics
#    (repair_design internally calls startIncremental/endIncremental)
estimate_parasitics -global_routing
repair_design

# 3. Fix overlaps introduced by repair_design
global_route -start_incremental
detailed_placement
global_route -end_incremental

# 4. Repair timing using global route parasitics
#    (repair_timing internally calls startIncremental/endIncremental)
estimate_parasitics -global_routing
repair_timing -setup

# 5. Fix overlaps introduced by repair_timing
global_route -start_incremental
detailed_placement
check_placement -verbose
global_route -end_incremental

puts "pass"
