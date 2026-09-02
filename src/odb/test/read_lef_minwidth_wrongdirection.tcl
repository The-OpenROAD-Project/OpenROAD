# Regression for issue #4252: LEF58_MINWIDTH WRONGDIRECTION with trailing whitespace
source "helpers.tcl"

set db [ord::get_db]
read_lef "data/lef58_minwidth_wrongdirection.lef"

set tech [$db getTech]
set layer [$tech findLayer M4]
if { $layer == "NULL" } {
  puts "FAIL: M4 layer not found"
  exit 1
}

set wrong_way_min_width [$layer getWrongWayMinWidth]
if { $wrong_way_min_width != 1000 } {
  puts "FAIL: wrong-way min width is $wrong_way_min_width, expected 1000"
  exit 1
}

puts "pass"
exit 0
