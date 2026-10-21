# repair_timing -path_group limits the critical VT swap to cells on the group's
# paths, even where another group's paths through other logic are worse.
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

# Late req_msg data makes in2reg violate through logic that no register
# drives. A reg2reg repair has no reason to swap any of it.
create_clock -name core_clock -period 180 [get_ports clk]
set_input_delay -clock core_clock 20 [all_inputs -no_clocks]
set_input_delay -clock core_clock 150 [get_ports req_msg*]
set_output_delay -clock core_clock 20 [all_outputs]
source asap7/setRC.tcl
estimate_parasitics -placement

rsz::resolve_path_group reg2reg
rsz::resolve_path_group in2reg

proc cell_refs { } {
  set refs {}
  foreach inst [get_cells *] {
    dict set refs [get_full_name $inst] [get_property $inst ref_name]
  }
  return $refs
}

# The cells some register launches into, the only ones reg2reg paths use.
set reg_fanout {}
foreach cell [get_fanout -from [all_registers -output_pins] -only_cells -flat] {
  dict set reg_fanout [get_full_name $cell] 1
}

set before [cell_refs]
foreach when {before after} {
  if { $when == "after" } {
    repair_timing -setup -phases CRIT_VT_SWAP -path_group reg2reg
  }
  foreach path_group {reg2reg in2reg} {
    puts "-- $path_group $when"
    report_checks -path_group $path_group -path_delay max -digits 3 \
      -fields {} -group_path_count 1 -format summary
  }
}

set outside {}
dict for {inst ref} [cell_refs] {
  if { $ref ne [dict get $before $inst] && ![dict exists $reg_fanout $inst] } {
    lappend outside $inst
  }
}
puts "VT swapped cells off every reg2reg path: [llength $outside]"
