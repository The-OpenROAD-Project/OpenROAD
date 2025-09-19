# test for via stacks that require a taper due to min width rules
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("asap7_vias/asap7_tech_1x.lef")
tech.readLef("asap7_vias/asap7sc7p5t_27_R_1x.lef")

design = helpers.make_design(tech)
design.readDef("asap7_vias/floorplan.def")

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDD$", power=True)
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VSS$", ground=True)

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")
pdn_aux.define_pdn_grid_real(design, name="top", voltage_domains=["Core"])

pdn_aux.add_pdn_stripe(
    design, grid="top", layer="M1", width=0.018, pitch=0.54, offset=0, followpins=True
)
pdn_aux.add_pdn_stripe(
    design, grid="top", layer="M2", width=0.018, pitch=0.54, offset=0, followpins=True
)
pdn_aux.add_pdn_stripe(
    design, grid="top", layer="M5", width=0.12, spacing=0.072, pitch=11.88, offset=0.300
)
pdn_aux.add_pdn_stripe(
    design, grid="top", layer="M6", width=0.288, spacing=0.096, pitch=12.0, offset=0.513
)

pdn_aux.add_pdn_connect(design, grid="top", layers=["M1", "M2"])
pdn_aux.add_pdn_connect(design, grid="top", layers=["M2", "M5"])
pdn_aux.add_pdn_connect(design, grid="top", layers=["M5", "M6"])

pdn_aux.pdngen_db(design)

def_file = helpers.make_result_file("asap7_taper.def")
design.writeDef(def_file)
helpers.diff_files("asap7_taper.defok", def_file)
