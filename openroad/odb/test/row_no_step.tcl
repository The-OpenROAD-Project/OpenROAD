# DEF ROW with an omitted STEP must default to abutting sites (DEF 5.8)
source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_def "data/row_no_step.def"

set block [ord::get_db_block]

proc row_info { block name } {
  foreach row [$block getRows] {
    if { [$row getName] == $name } {
      set bbox [$row getBBox]
      return [list [$row getSpacing] \
        [$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]]
    }
  }
}

# Site is 0.19 x 1.4 um = 380 x 2800 DBU
check "h_implicit" {row_info $block h_implicit} \
  "380 10000 10000 13800 12800"
check "h_explicit" {row_info $block h_explicit} \
  "380 10000 20000 13800 22800"
check "v_implicit" {row_info $block v_implicit} \
  "2800 40000 10000 40380 38000"
check "v_explicit" {row_info $block v_explicit} \
  "2800 50000 10000 50380 38000"
# W (R90) swaps the site extents, so horizontal pitch is the site height
check "h_implicit_w" {row_info $block h_implicit_w} \
  "2800 60000 10000 88000 10380"
# An explicit zero STEP must be kept, not treated as omitted
check "single_explicit_zero" {row_info $block single_explicit_zero} \
  "0 70000 10000 70380 12800"
check "single_implicit" {row_info $block single_implicit} \
  "380 80000 10000 80380 12800"

set def_file [make_result_file row_no_step.def]
write_def $def_file
diff_files row_no_step.defok $def_file

exit_summary
