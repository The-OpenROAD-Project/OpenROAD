# Fresh session for estimate_parasitics_droute_guides.tcl: its guides are
# marked as written by detailed routing.
source "helpers.tcl"
read_liberty "sky130hs/sky130hs_tt.lib"
read_db [make_result_file estimate_parasitics_droute_guides.odb]
source "sky130hs/sky130hs.rc"
set_wire_rc -signal -layer met2
set_wire_rc -clock -layer met5

set total 0
set marked 0
foreach guide [[[ord::get_db_block] findNet clk] getGuides] {
  incr total
  if { [$guide isFromDetailedRoute] } {
    incr marked
  }
}
check "the guide marks survive the database round trip" {
  expr { $total > 0 && $marked == $total }
} 1

check_error "-global_routing rejects guides from detailed routing" {
  estimate_parasitics -global_routing
} EST-0021

estimate_parasitics
check "the flagless estimate falls back to placement" {
  est::parasitics_source
} placement

# The automatic pick in find_resize_slacks falls back too, not errors.
check "find_resize_slacks does not use the detailed routing guides" {
  catch { rsz::find_resize_slacks }
} 0
check "find_resize_slacks keeps the placement source" {
  est::parasitics_source
} placement

# A rejected source leaves the parasitics it found untouched.
set spef_file [make_result_file estimate_parasitics_droute_guides.spef]
estimate_parasitics -placement -spef_file $spef_file
set spef_size [file size $spef_file]
read_spef $spef_file
check_error "a rejected source does not disturb the SPEF" {
  estimate_parasitics -global_routing
} EST-0021
check "the parasitics still come from the SPEF" {
  est::parasitics_source
} spef
check "a rejected source does not truncate the SPEF file" {
  expr { [file size $spef_file] == $spef_size }
} 1

exit_summary
