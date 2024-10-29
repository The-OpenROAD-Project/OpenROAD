from openroad import Design, Tech
import pdn_aux
import helpers
from collections import defaultdict

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("nangate_macros/fakeram45_64x32.lef")

design = helpers.make_design(tech)
design.readDef("nangate_macros/floorplan.def")

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDD$", power=True)

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDDPE$")

pdn_aux.add_global_connection(design, net_name="VDD", pin_pattern="^VDDCE$")

pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VSS$", ground=True)

pdn_aux.add_global_connection(design, net_name="VSS", pin_pattern="^VDDE$")

pdn_aux.set_voltage_domain(design, power="VDD", ground="VSS")

pdn_aux.define_pdn_grid_real(design, name="Core")
pdn_aux.add_pdn_stripe(design, layer="metal1", followpins=True)
pdn_aux.add_pdn_stripe(
    design, layer="metal4", width=0.48, spacing=4.0, pitch=49.0, offset=2.0
)
pdn_aux.add_pdn_stripe(design, layer="metal7", width=1.4, pitch=40.0, offset=2.0)

pdn_aux.add_pdn_connect(design, layers=["metal1", "metal4"])
pdn_aux.add_pdn_connect(design, layers=["metal4", "metal7"])

pdn_aux.define_pdn_grid_macro(
    design, name="sram1", instances=["dcache.data.data_arrays_0.data_arrays_0_ext.mem"]
)
pdn_aux.add_pdn_stripe(design, layer="metal5", width=0.93, pitch=10.0, offset=2.0)
pdn_aux.add_pdn_stripe(design, layer="metal6", width=0.93, pitch=10.0, offset=2.0)

pdn_aux.add_pdn_connect(design, layers=["metal4", "metal5"])
pdn_aux.add_pdn_connect(design, layers=["metal5", "metal6"])
pdn_aux.add_pdn_connect(design, layers=["metal6", "metal7"])

pdn_aux.define_pdn_grid_macro(
    design,
    name="sram2",
    instances=["frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem"],
    is_bump=False,
)

pdn_aux.add_pdn_stripe(design, layer="metal5", width=0.93, pitch=10.0, offset=2.0)
pdn_aux.add_pdn_stripe(design, layer="metal6", width=0.93, pitch=10.0, offset=2.0)

pdn_aux.add_pdn_connect(design, layers=["metal4", "metal5"])
pdn_aux.add_pdn_connect(design, layers=["metal5", "metal6"])
pdn_aux.add_pdn_connect(design, layers=["metal6", "metal7"])

pdn_aux.pdngen_db(design)

def_file = helpers.make_result_file("macros.def")
design.writeDef(def_file)
helpers.diff_files("macros.defok", def_file)
