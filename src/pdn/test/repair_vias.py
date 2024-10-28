# test for repairing vias
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("asap7_vias/asap7_tech_1x_noviarules.lef")
tech.readLef("asap7_vias/asap7sc7p5t_27_R_1x.lef")
design = helpers.make_design(tech)
design.readDef("asap7_vias/floorplan_repair.def")

pdn_aux.repair_pdn_vias(design, all=True)

def_file = helpers.make_result_file("repair_vias.def")
design.writeDef(def_file)
helpers.diff_files("repair_vias.defok", def_file)
