from openroad import Design, Tech
import helpers
import rcx_aux

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
tech.readLef("sky130hs/sky130hs_std_cell.lef")
tech.readLiberty("sky130hs/sky130hs_tt.lib")

test_nets = ""

design = helpers.make_design(tech)
design.readDef("gcd.def")

# This uses 'set_layer_rc' which is in rsz which has not yet been
# Python wrapped. Remove when rsz has been wrapped
design.evalTclString("source sky130hs/sky130hs.rc")

rcx_aux.define_process_corner(design, ext_model_index=0, filename="X")

rcx_aux.extract_parasitics(
    design, ext_model_file="ext_pattern.rules", max_res=0, coupling_threshold=0.1
)

spef_file = helpers.make_result_file("gcd.spef")
rcx_aux.write_spef(design, filename=spef_file, nets=test_nets)
helpers.diff_files("gcd.spefok", spef_file, "^\\*(DATE|VERSION)")
