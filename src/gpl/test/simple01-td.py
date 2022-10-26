from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLiberty("./library/nangate45/NangateOpenCellLibrary_typical.lib")
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("./simple01-td.def")

design.evalTclString("create_clock -name core_clock -period 2 clk")

design.evalTclString("set_wire_rc -signal -layer metal3")
design.evalTclString("set_wire_rc -clock  -layer metal5")

gpl_aux.global_placement(design, timing_driven=True)

design.evalTclString("estimate_parasitics -placement")
design.evalTclString("report_worst_slack")

def_file = helpers.make_result_file("simple01-td.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-td.defok")


# source helpers.tcl
# set test_name simple01-td
# read_liberty ./library/nangate45/NangateOpenCellLibrary_typical.lib

# read_lef ./nangate45.lef
# read_def ./$test_name.def

# create_clock -name core_clock -period 2 clk

# set_wire_rc -signal -layer metal3
# set_wire_rc -clock  -layer metal5

# global_placement -timing_driven

# # check reported wns
# estimate_parasitics -placement
# report_worst_slack

# set def_file [make_result_file $test_name.def]
# write_def $def_file
# diff_file $def_file $test_name.defok
