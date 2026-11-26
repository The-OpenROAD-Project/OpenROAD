# The Python repair_timing reports an unknown -sequence move as RSZ-3282,
# like the Tcl command (see repair_timing_option_errors.tcl).
from openroad import Tech
import helpers
import rsz_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("report_delay_estimator_rebuffer_minimal.def")
design.evalTclString("create_clock -period 0.3 clk")

design.evalTclString("source Nangate45/Nangate45.rc")
design.evalTclString("set_wire_rc -layer metal3")
design.evalTclString("estimate_parasitics -placement")

try:
    rsz_aux.repair_timing(design, setup=True, hold=False, sequence="sizeup,bogus")
except Exception as inst:
    print(inst.args[0])
