# Check that read_upf errors cross the C++/Tcl boundary without aborting.
set test_name read_upf_errors
source "helpers.tcl"

read_liberty data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef data/sky130hd/sky130_fd_sc_hd.tlef
read_lef data/sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog data/mpd_top/mpd_top.v
link_design mpd_top

read_upf -file data/mpd_top/mpd_top.upf

set upf_file [make_result_file "${test_name}.upf"]

foreach {description command expected_error} {
  "missing master"
  {map_power_switch psw_aes_1 -lib_cells {no_such_cell} -port_map {{vin1 A}}}
  UPF-0043

  "missing master terminal"
  {map_power_switch psw_aes_1 -lib_cells {sky130_fd_sc_hd__and2_0} -port_map {{vin1 NOPE}}}
  ODB-0154

  "missing isolation domain"
  {set_isolation iso_x}
  UPF-0073
} {
  set ::read_upf_after_error 0
  set stream [open $upf_file w]
  puts $stream $command
  puts $stream {set ::read_upf_after_error 1}
  close $stream

  set status [catch { read_upf -file $upf_file } message]
  check "$description returns a Tcl error" { set status } 1
  check "$description preserves the error ID" { set message } $expected_error
  check "$description stops the UPF file" { set ::read_upf_after_error } 0

  set stream [open $upf_file w]
  puts $stream {
    map_power_switch psw_aes_1 \
      -lib_cells {sky130_fd_sc_hd__and2_0} \
      -port_map {{vin1 A}}
  }
  puts $stream {set ::read_upf_after_error 1}
  close $stream

  check "valid UPF succeeds after $description" {
    catch { read_upf -file $upf_file }
  } 0
  check "valid UPF completes after $description" { set ::read_upf_after_error } 1
}

exit_summary
