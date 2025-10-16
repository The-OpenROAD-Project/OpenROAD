# test for pdngen -reset
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = helpers.make_design(tech)
design.readDef("nangate_gcd/floorplan.def")
pdngen = design.getPdnGen()

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="VDD", power=True)
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="VSS", ground=True)

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")

pdn_aux.define_pdn_grid_real(design, name="Core")
pdn_aux.add_pdn_stripe(design, followpins=True, layer="metal1")

pdn_aux.pdngen_db(design)

def_file = helpers.make_result_file("core_grid.def")
design.writeDef(def_file)
helpers.diff_files("core_grid.defok", def_file)
