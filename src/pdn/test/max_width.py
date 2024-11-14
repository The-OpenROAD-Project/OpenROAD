# test for max width violation
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("asap7_vias/asap7_tech_1x.lef")
tech.readLef("asap7_vias/asap7sc7p5t_27_R_1x.lef")

design = helpers.make_design(tech)
design.readDef("asap7_vias/floorplan.def")
pdngen = design.getPdnGen()

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDD$", power=True)
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VSS$", ground=True)
pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")
pdn_aux.define_pdn_grid_real(design, name="Core")

try:
    pdn_aux.add_pdn_stripe(
        design, layer="M8", width=4.0, spacing=2.688, pitch=56.32, offset=6.408
    )
except Exception as inst:
    print(inst.args[0])
