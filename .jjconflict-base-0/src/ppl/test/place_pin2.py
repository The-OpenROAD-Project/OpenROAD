# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = helpers.make_design(tech)
design.readDef("gcd.def")

ppl_aux.place_pin(
    design, pin_name="clk", layer="metal7", location=[40, 30], pin_size=[1.6, 2.5]
)

ppl_aux.place_pin(
    design, pin_name="resp_val", layer="metal4", location=[12, 50], pin_size=[2, 2]
)
ppl_aux.place_pin(
    design,
    pin_name=r"req_msg\[0\]",
    layer="metal10",
    location=[25, 70],
    pin_size=[4, 4],
)

ppl_aux.place_pins(
    design,
    hor_layers="metal3",
    ver_layers="metal2",
    corner_avoidance=0,
    min_distance=0.12,
)

def_file = helpers.make_result_file("place_pin2.def")
design.writeDef(def_file)
helpers.diff_files("place_pin2.defok", def_file)
