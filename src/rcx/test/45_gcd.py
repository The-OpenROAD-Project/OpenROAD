from openroad import Design, Tech
import rcx_aux
import helpers
import via_45_resistance as via_45

test_nets = ""

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")

design = helpers.make_design(tech)
design.readDef("45_gcd.def")

# Load via resistance info
via_45.set_resistance(tech)

rcx_aux.define_process_corner(design, ext_model_index=0, filename="X")

rcx_aux.extract_parasitics(
    design, ext_model_file="45_patterns.rules", max_res=0, coupling_threshold=0.1
)

spef_file = helpers.make_result_file("45_gcd.spef")
rcx_aux.write_spef(design, filename=spef_file, nets=test_nets)

helpers.diff_files("45_gcd.spefok", spef_file, "^\\*(DATE|VERSION)")
