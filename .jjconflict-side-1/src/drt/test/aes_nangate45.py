# Since aes_nangate45 is not in the regressions that are run (no ok or defok
# file, and multiple places is code where they print run dependent info
# directly to stdout without going through the logging interface) this Python
# regression is here as an example only (note the use of the global router
# for reading in the guide file and also using tech to set the number of
# threads for multi-threaded execution)
from openroad import Design, Tech, set_thread_count
import helpers
import drt_aux
import grt
import utl

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")
design = helpers.make_design(tech)
design.readDef("aes_nangate45_preroute.def")

gr = design.getGlobalRouter()
gr.readGuides("aes_nangate45.route_guide")
set_thread_count(4)

drt_aux.detailed_route(
    design,
    output_drc="results/aes_nangate45.output.drc-py.rpt",
    output_maze="results/aes_nangate45.output.maze-py.log",
    verbose=1,
)

def_file = helpers.make_result_file("aes_nangate45.def")
design.writeDef(def_file)
