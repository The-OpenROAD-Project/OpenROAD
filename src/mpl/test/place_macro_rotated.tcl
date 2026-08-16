# place_macro must validate core containment against the footprint the macro
# will have AFTER the requested orientation is applied.
#
# RECT_MACRO_100x400 is 100um x 400um and the core is 499.89um x 198.80um, so:
#   - R90 (400 x 100) fits;
#   - R0  (100 x 400) does not.
# Checking the pre-setOrient bbox rejected the legal R90 placement with
# MPL-0034 and would symmetrically have accepted an illegal one.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/place_macro_rotated.lef"
read_def "./testcases/place_macro_rotated.def"

# The regression: this is legal at R90 and must be accepted.
place_macro -macro_name macro -location {20 20} -orientation R90

# The guard against over-correcting: the same origin is genuinely outside the
# core at R0, and must still be rejected.
if { [catch { place_macro -macro_name macro -location {20 20} -orientation R0 } msg] } {
  puts "R0 correctly rejected"
} else {
  puts "ERROR: R0 placement should have been rejected"
}

set def_file [make_result_file place_macro_rotated.def]
write_def $def_file
diff_files place_macro_rotated.defok $def_file
