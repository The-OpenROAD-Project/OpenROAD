from openroad import Design, Tech, set_thread_count
import helpers
import rmp_aux

tech = Tech()
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("rcon.def")
design.evalTclString("read_sdc rcon.sdc")
design.evalTclString("report_design_area")

tiehi = "LOGIC1_X1/Z"
tielo = "LOGIC0_X1/Z"

rmp_aux.restructure(
    design,
    liberty_file_name="Nangate45/Nangate45_typ.lib",
    target="area",
    workdir_name="results/python/const",
    abc_logfile="results/abc_rcon.log",
    tielo_port=tielo,
    tiehi_port=tiehi,
)

design.evalTclString("report_design_area")
