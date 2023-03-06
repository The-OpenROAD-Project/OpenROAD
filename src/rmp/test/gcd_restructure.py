from openroad import Design, Tech, set_thread_count
import helpers
import rmp_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = Design(tech)
design.readDef("gcd_placed.def")

# read_sdc is defined in sta/tcl/Sdc.tcl (not yet wrapped)
design.evalTclString("read_sdc gcd.sdc")

# set_wire_rc and estimate_parasitics are both defined in rsz (not yet wrapped)
design.evalTclString("set_wire_rc -layer metal3")
design.evalTclString("estimate_parasitics -placement")
design.evalTclString("report_worst_slack")
design.evalTclString("report_design_area")

tiehi = "LOGIC1_X1/Z"
tielo = "LOGIC0_X1/Z"

set_thread_count(1)

rmp_aux.restructure(design, liberty_file_name="Nangate45/Nangate45_typ.lib", target="area",
                    abc_logfile="results/abc_rcon.log", tielo_port=tielo, tiehi_port=tiehi,
                    workdir_name="./results")

design.evalTclString("report_design_area")
