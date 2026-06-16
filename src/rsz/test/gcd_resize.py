from openroad import Tech
import helpers
import rsz_aux

helpers.if_bazel_change_working_dir_to("/_main/src/rsz/test/")

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("gcd_nangate45_placed.def")
design.evalTclString("read_sdc gcd_nangate45.sdc")

design.evalTclString("source Nangate45/Nangate45.rc")
design.evalTclString("set_wire_rc -layer metal3")
design.evalTclString("estimate_parasitics -placement")

design.evalTclString("report_worst_slack")

# rsz_aux.set_dont_use() requires resolved sta::LibertyCell* objects.
# Liberty cell lookup is not yet available from the Python STA API;
# use evalTclString until sta is wrapped (see rmp_aux.py for the same pattern).
design.evalTclString("set_dont_use {AOI211_X1 OAI211_X1}")

rsz_aux.buffer_ports(design)

rsz_aux.repair_design(design)

# rsz_aux.repair_tie_fanout() requires a resolved sta::LibertyPort* object.
# Liberty port lookup is not yet available from the Python STA API;
# use evalTclString until sta is wrapped (see rmp_aux.py for the same pattern).
design.evalTclString("repair_tie_fanout LOGIC0_X1/Z")
design.evalTclString("repair_tie_fanout LOGIC1_X1/Z")

rsz_aux.repair_timing(design, setup=True, hold=False)
rsz_aux.repair_timing(design, setup=False, hold=True, hold_margin=0.2)

design.evalTclString("report_worst_slack -min")
design.evalTclString("report_worst_slack -max")
design.evalTclString("report_check_types -max_slew -max_fanout -max_capacitance")

rsz_aux.report_long_wires(design, 10)

rsz_aux.report_floating_nets(design)

rsz_aux.report_design_area(design)

def_file = helpers.make_result_file("gcd_resize.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "gcd_resize.defok")
