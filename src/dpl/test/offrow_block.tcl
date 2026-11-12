# Shared by the check_offrow_block_* tests: move the fixed block "ram" and
# the cell "inv", run check_placement and compare the outcome to expected.
proc offrow_scenario { description ram_xy inv_xy inv_orient expected } {
  set block [ord::get_db_block]
  set ram [$block findInst ram]
  $ram setPlacementStatus PLACED
  $ram setLocation {*}$ram_xy
  $ram setPlacementStatus FIRM
  set inv [$block findInst inv]
  # dbInst::setOrient moves the origin, so orient before setting location.
  $inv setOrient $inv_orient
  $inv setLocation {*}$inv_xy
  puts "== $description: ram at $ram_xy, inv at $inv_xy $inv_orient"
  if { [catch { check_placement -verbose }] } {
    set result fail
  } else {
    set result pass
  }
  puts "result: $result"
  check $description { set result } $expected
}
