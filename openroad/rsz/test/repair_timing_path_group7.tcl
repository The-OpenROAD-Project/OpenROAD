# repair_timing -path_group takes a group the SDC defines (group_path): it
# repairs the group's violating endpoints, judged by the group's slack. A
# net a group runs -through can be deleted without leaving the group
# pointing at it.
source "helpers.tcl"
source asap7/asap7.vars
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_AO_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_OA_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_SIMPLE_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_SEQ_${vt}_FF_nldm_220123.lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def

create_clock -name core_clock -period 180 [get_ports clk]
set_input_delay -clock core_clock 20 [all_inputs -no_clocks]
set_output_delay -clock core_clock 20 [all_outputs]
source asap7/setRC.tcl
estimate_parasitics -placement

# The group: paths through the worst path's register output net.
set worst [find_timing_paths -path_delay max -group_path_count 1]
set start_pin [get_property [get_property $worst startpoint] full_name]
set thru_net [get_nets -of_objects [get_pins $start_pin]]
puts "group through [get_full_name $thru_net]"
group_path -name thru -through $thru_net

proc group_violations { } {
  set ends [find_timing_paths -path_group thru -path_delay max \
    -group_path_count 1000 -endpoint_path_count 1 -unique_paths_to_endpoint \
    -slack_max 0]
  set wns 0
  foreach end $ends { set wns [expr { min($wns, [get_property $end slack]) }] }
  return [list [llength $ends] [format %.3f $wns]]
}
lassign [group_violations] n_before wns_before
set all_before [llength [find_timing_paths -path_delay max -group_path_count 1000 \
  -endpoint_path_count 1 -unique_paths_to_endpoint -slack_max 0]]
puts "violating endpoints: $all_before in all, $n_before in the group (wns $wns_before)"
repair_timing -setup -path_group thru
lassign [group_violations] n_after wns_after
set improved [expr { $wns_after > $wns_before }]
puts "group after: $n_after violating, wns $wns_after, improved: $improved"

# A net a group runs -through is deleted, as a resizer move deletes one
# (through OpenDB's net destroy): the group no longer names it.
set doomed [odb::dbNet_create [ord::get_db_block] doomed]
group_path -name doomed_group -through [get_nets doomed]
odb::dbNet_destroy $doomed
puts "net still there: [llength [get_nets -quiet doomed]]"
set sdc [make_result_file repair_timing_path_group7.sdc]
write_sdc -no_timestamp $sdc
set f [open $sdc]
set text [read $f]
close $f
# The group's statement, up to the next command, if the group is left:
# no net in it.
set group ""
regexp {group_path -name doomed_group[^\n]*(\\\n[^\n]*)*} $text group
puts "doomed_group names a net: [regexp {get_nets} $group]"
