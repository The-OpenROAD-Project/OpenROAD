# gcd_nangate45 IO placement
from openroad import Design, Tech
import helpers
import ppl_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
design = Design(tech)
design.readDef("gcd.def")

ppl_aux.place_pins(design, hor_layers="metal3",
                   ver_layers="metal2",
                   corner_avoidance=0, min_distance=1,
                   min_distance_in_tracks=True)

def_file = helpers.make_result_file("min_dist_in_tracks1.def")
design.writeDef(def_file)
helpers.diff_files("min_dist_in_tracks1.defok", def_file)
