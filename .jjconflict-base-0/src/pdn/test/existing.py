from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("nangate_macros/fakeram45_64x32.lef")

design = helpers.make_design(tech)
design.readDef("nangate_existing/floorplan.def")


pdn_aux.define_pdn_grid_existing(design)


pdn_aux.add_pdn_connect(design, layers=["metal1", "metal4"])
pdn_aux.add_pdn_connect(design, layers=["metal4", "metal7"])
pdn_aux.add_pdn_connect(design, layers=["metal7", "metal8"])

pdn_aux.add_pdn_connect(design, layers=["metal4", "metal5"])
pdn_aux.add_pdn_connect(design, layers=["metal5", "metal6"])
pdn_aux.add_pdn_connect(design, layers=["metal6", "metal7"])

pdn_aux.pdngen_db(design)


def_file = helpers.make_result_file("existing.def")
design.writeDef(def_file)
helpers.diff_files("existing.defok", def_file)
