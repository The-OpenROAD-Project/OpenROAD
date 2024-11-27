# Test for building bump array
source "helpers.tcl"

# Init chip
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan.def

catch {[make_io_bump_array -bump DUMMY_BUMP -origin "200" -rows 14 -columns 14 -pitch "200 200"]} err
puts $err

catch {[make_io_bump_array -bump DUMMY_BUMP -origin "200 200 200" -rows 14 -columns 14 -pitch "200 200"]} err
puts $err

catch {[make_io_bump_array -bump DUMMY_BUMP -origin "200 200" -rows 14 -columns 14 -pitch "200 200 200"]} err
puts $err

catch {[make_io_bump_array -bump DUMMY_BUMP -origin "200 200" -rows 14 -columns 14 -pitch ""]} err
puts $err
