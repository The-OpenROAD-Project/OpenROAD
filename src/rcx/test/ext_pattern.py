from openroad import Design, Tech
import helpers
import rcx_aux

test_nets = "3 48 92 193 200 243 400 521 671"

tech = Tech()
tech.readLef("sky130hs/sky130hs.tlef")
design = helpers.make_design(tech)
design.readDef("generate_pattern.defok")

rcx_aux.define_process_corner(design, ext_model_index=0, filename="X")

rcx_aux.extract_parasitics(
    design,
    ext_model_file="ext_pattern.rules",
    cc_model=12,
    max_res=0,
    context_depth=10,
    coupling_threshold=0.1,
)

spef_file = helpers.make_result_file("ext_pattern.spef")
rcx_aux.write_spef(design, filename=spef_file, nets=test_nets)
helpers.diff_files("ext_pattern.spefok", spef_file, "^\\*(DATE|VERSION)")
