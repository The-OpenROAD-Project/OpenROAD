# test for grid checking if inputs are on the manufacturing grid
from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")

design = helpers.make_design(tech)
design.readDef("nangate_gcd/floorplan.def")

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="VDD", power=True)
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="VSS", ground=True)

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")
pdn_aux.define_pdn_grid_real(design, name="Core")

try:
    pdn_aux.add_pdn_ring(
        design,
        grid="Core",
        layers=["metal5", "metal6"],
        widths=2 * [2.001],
        spacings=2 * [2.0],
        core_offsets=4 * [2.0],
    )
except Exception as inst:
    print(inst.args[0])
