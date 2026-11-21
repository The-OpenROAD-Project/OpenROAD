# report_long_wires prints at most the requested number of wires.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def
read_sdc gcd_nangate45.sdc
set_wire_rc -signal -layer metal3
set_wire_rc -clock -layer metal5
estimate_parasitics -placement

foreach count {0 1 3 10} {
  set report_file "long_wires_$count.rpt"
  report_long_wires $count > $report_file
  set fh [open $report_file r]
  set lines [split [read $fh] "\n"]
  close $fh
  set rows [expr { [llength [lsearch -all -inline -not -exact $lines ""]] - 1 }]
  puts "count=$count rows=$rows"
}
