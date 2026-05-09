from openroad import Tech
import helpers
import pdn_aux

bazel_working_dir = "/_main/src/pdn/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("Nangate45/Nangate45_tech.lef")
tech.readLef("Nangate45/Nangate45_stdcell.lef")

design = helpers.make_design(tech)
design.readDef("nangate_gcd/floorplan.def")

# Create the database nets in the opposite order from the voltage domain.  PDN
# should still create special wires in voltage-domain order, not dbNet pointer
# order.
pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="VSS", ground=True)
pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="VDD", power=True)

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")

pdn_aux.define_pdn_grid_real(design, name="Core")
pdn_aux.add_pdn_stripe(design, followpins=True, layer="metal1")

pdn_aux.pdngen_db(design)

block = design.getBlock()
vdd_swire = next(iter(block.findNet("VDD").getSWires()))
vss_swire = next(iter(block.findNet("VSS").getSWires()))

if vdd_swire.getId() > vss_swire.getId():
    raise AssertionError("PDN special wires were created in dbNet pointer order")

print("PDN swire order follows voltage domain order.")
