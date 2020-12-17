
# load lef and deef
read_lef "LEF file"
read_def -order_wires "DEF file" 


proc add_antenna_cell { net antenna_cell_name sink_inst sink_iterm antenna_inst_name} {

  set block [[[::ord::get_db] getChip] getBlock]
  set net_name [$net getName]

  set antenna_master [[::ord::get_db] findMaster $antenna_cell_name]
  set antenna_mterm [$antenna_master getMTerms]

  set inst_loc [$sink_inst getLocation]
  set inst_loc_x [lindex [$sink_inst getLocation] 0]
  set inst_loc_y [lindex [$sink_inst getLocation] 1]
  set inst_ori [$sink_inst getOrient]

  set antenna_inst [odb::dbInst_create $block $antenna_master $antenna_inst_name]
  set antenna_iterm [$antenna_inst findITerm "A"]

  $antenna_inst setLocation $inst_loc_x $inst_loc_y
  $antenna_inst setOrient $inst_ori
  $antenna_inst setPlacementStatus PLACED
  odb::dbITerm_connect $antenna_iterm $net
}


set db [::ord::get_db]
set block [[$db getChip] getBlock]


foreach inst [$block getInsts] {
  if {[[$inst getMaster] getType] == "CORE_SPACER"} {
    odb::dbInst_destroy $inst
  }
}

set insts [$block getInsts]

foreach inst $insts {
  dict set inst_count $inst 0
}

set antenna_cell_name "ANTENNA3_A9PP84TR_C14"

set target_file "design_with_diodes"

# ==================================================
# Insert ANTENNA diode on every pin
# =================================================
puts "start to read nets"

set iterate_times 0

set antenna_node_counts 0

while { $iterate_times < 1 } {
  puts "iterate: [expr $iterate_times + 1] times"
  
  set nets [$block getNets]

  foreach net $nets {
    set flag [check_net_violation -net_name [$net getConstName]] 
    if {$flag == 0} {
      continue
    } else {
      #puts "adding diodes on net: [$net getName]"
    }
  
    if { [$net isSpecial] } {
      continue
    }
  
    #set iterm [antennachecker::getViolationITerm $net]
    foreach iterm [$net getITerms] {
      set inst [$iterm getInst]
  
      dict set inst_count $inst [expr [dict get $inst_count $inst] + 1]
  
      set count [dict get $inst_count $inst]
      
      set antenna_inst_name "ANTENNA"
      append antenna_inst_name "_" [$inst getName] "_" $count
   
      if {[catch {add_antenna_cell $net $antenna_cell_name $inst $iterm $antenna_inst_name} result] } {
        puts "adding node failed"
        continue
      } else {
        set antenna_inst [$block findInst $antenna_inst_name]
        dict set inst_count $antenna_inst 1 
        set antenna_node_counts [expr $antenna_node_counts + 1]
      }
  
      break  
   
    }
  }
  
  set iterate_times [expr $iterate_times + 1]

}

puts "end reading"

# =================================================
# Finish insertion
# ================================================

detail_place


set verilog_file_name "$target_file.v"
write_verilog  $verilog_file_name

set def_file_name "$target_file.def"
write_def $def_file_name

