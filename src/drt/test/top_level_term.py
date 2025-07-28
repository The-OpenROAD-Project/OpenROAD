from openroad import Design, Tech
import helpers
import drt_aux

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")
design = helpers.make_design(tech)
design.readDef("top_level_term.def")

gr = design.getGlobalRouter()
gr.readGuides("top_level_term.guide")

design.evalTclString("set_routing_layers -signal met1-met3")

drt_aux.detailed_route(design, verbose=0)

def_file = helpers.make_result_file("top_level_term.def")
design.writeDef(def_file)
helpers.diff_files("top_level_term.defok", def_file)
