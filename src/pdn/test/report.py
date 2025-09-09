from openroad import Design, Tech
import pdn_aux
import helpers

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("nangate_bsg_black_parrot/dummy_pads.lef")

design = helpers.make_design(tech)
design.readDef("nangate_bsg_black_parrot/floorplan.def")
pdngen = design.getPdnGen()

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDD$", power=True)
pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDDPE$")
pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDDCE$")
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VSS$", ground=True)
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VSSE$")

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")

pdn_aux.define_pdn_grid_real(design, name="Core")
pdn_aux.add_pdn_ring(
    design,
    grid="Core",
    layers=["metal8", "metal9"],
    widths=2 * [5.0],
    spacings=2 * [2.0],
    core_offsets=4 * [4.5],
    connect_to_pads=True,
)
pdn_aux.add_pdn_stripe(
    design, followpins=True, layer="metal1", extend_to_core_ring=True
)

pdn_aux.add_pdn_stripe(
    design, layer="metal4", width=0.48, pitch=56.0, offset=2.0, extend_to_core_ring=True
)
pdn_aux.add_pdn_stripe(
    design, layer="metal7", width=1.40, pitch=40.0, offset=2.0, extend_to_core_ring=True
)
pdn_aux.add_pdn_stripe(
    design, layer="metal8", width=1.40, pitch=40.0, offset=2.0, extend_to_core_ring=True
)
pdn_aux.add_pdn_stripe(
    design, layer="metal9", width=1.40, pitch=40.0, offset=2.0, extend_to_core_ring=True
)

pdn_aux.add_pdn_connect(design, layers=["metal1", "metal4"])
pdn_aux.add_pdn_connect(design, layers=["metal4", "metal7"])
pdn_aux.add_pdn_connect(design, layers=["metal7", "metal8"])
pdn_aux.add_pdn_connect(design, layers=["metal8", "metal9"])

pdngen.report()
