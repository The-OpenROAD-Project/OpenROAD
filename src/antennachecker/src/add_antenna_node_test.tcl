#read_liberty example1_typ.lib
#read_lef example1.lef
#read_def example1.def

#read_lef tech.lef
#read_lef test.lef
#read_def -order_wires gcd.def

#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/sc9mcpp84_tech.lef
#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/sc9mcpp84_14lpp_base_rvt_c14.lef
#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/gf14lpp_2rf_lg5_w32_all.lef
#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/gf14lpp_1rw_lg10_w32_all.lef
#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/gf14lpp_1rw_lg10_w32_byte.lef
#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/libs/lef/io_gppr_14lpp_t18_mv10_fs18_rvt_dr_9M_3Mx_2Cx_2Kx_2Gx_LB.lef
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/vb/bsg_manycore_tile_antenna.def

#read_lef NangateOpenCellLibrary.macro.lef
#read_lef NangateOpenCellLibrary.macro.mod.lef
#read_lef NangateOpenCellLibrary.macro.rect.lef
#read_lef NangateOpenCellLibrary.macro.tech.lef
#read_def -order_wires 5_route.def


#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/swerve/merged.lef
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/swerve/5_route.def
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/testcases/aes/5_route.def


#read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/openlane-testcases/fastroute_max_routing/merged_unpadded.lef
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/restricted/google/mehdi/openlane-testcases/TR-Antenna/APU.routing.def
#set db1 [odb::dbDatabase_create]
#set db2 [odb::dbDatabase_create]
#set db1 [odb::dbDatabase_create]
#odb::read_lef $db1 "/afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/generic_merged_spacing.lef"
#odb::read_def $db1 "/afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/OpenROAD/src/antennachecker/test/6_final.def"

#odb::read_lef $db2 "/afs/eecs.umich.edu/vlsida/projects/restricted/google/mehdi/OpenROAD-flow/flow/objects/gf14/bp_single/generic_merged_spacing.lef"
#odb::read_def $db2 "/afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/OpenROAD/src/antennachecker/test/3_3_place_dp.def"

#read_lef /afs/eecs.umich.edu/vlsida/projects/restricted/google/mehdi/OpenROAD-flow/flow/objects/gf14/bp_single/generic_merged_spacing.lef
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/OpenROAD-flow/flow/results/gf14/bsg_loopback/6_final.def 

read_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/generic_merged_spacing.lef
read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/OpenROAD-flow_0621/flow/results/gf14/bp_single/6_final.def

#set verilog_file "write_verilog1.v"
#write_verilog  $verilog_file



#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/mehdi/antennachecker/OpenROAD-flow/flow/results/gf14/bp_single/6_final.def

# add_antenna_cell { net antenna_cell_name sink_inst sink_iterm antenna_inst_name }

#set db1 [::ord::get_db]
#set tech [$db getTech]
#set block [[$db getChip] getBlock]

#set inst_name "next.downlink.ch_0__downstream.baf.bapg_rd.w_ptr_r_reg_1_"
#set inst [$block findInst $inst_name]


#set inst_lo [$inst getLocation]
#set inst_loc_x [lindex [$inst getLocation] 0]
#set inst_loc_y [lindex [$inst getLocation] 1]
#set inst_ori [$inst getOrient]

#puts $inst_lo
#puts $inst_loc_x
#puts $inst_loc_y
#puts $inst_ori
#set antenna_cell_name "ANTENNA3_A9PP84TR_C14"
#set antenna_master [$db findMaster $antenna_cell_name]


#set viofilename "/afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/rpt_comp_script/inno_vio_nets.txt"

#set data [json::json2dict $viofilename]

#puts [dict size $data]

#set nets [$block getNets]
#foreach net $nets {
# # set flag [antennachecker::checkViolation $net]
# # if ($flag == 0) {
# #   puts "no violation"
# #   continue
# # } else {
# #   puts "violation detected!"
# # }
#
#  foreach iterm [$net getITerms] {
#    set antenna_inst_name "antenna_node"
#    append antenna_inst_name "_" [$net getName] "_" [[$iterm getInst] getConstName]
#    set inst [$iterm getInst]
#    add_antenna_cell $net $antenna_cell_name $inst $iterm $antenna_inst_name   
#    if ([catch {add_antenna_cell $net $antenna_cell_name $inst $iterm $antenna_inst_name} result]) {
#        puts "error occurs"
#        continue       
#    } else {
#        puts "antenna node added"
#    }
#  } 
#}


#set count [antennachecker::getAllNets $db]
#puts "inst count: $count"

#puts antennachecker::getAllNets $db

set db [::ord::get_db]
set block [[$db getChip] getBlock]
#set block2 [[$db1 getChip] getBlock]

#puts "def1 insts nums: [llength [$block getInsts]]"
#puts "def1 nets nums: [llength [$block getNets]]"
#
#read_def -order_wires /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/OpenROAD/src/antennachecker/test/6_final.def
#
#puts "def2 inst nums: [llength [$block getInsts]]"
#puts "def2 nets nums: [llength [$block getNets]]"
#exit 0



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

set target_file "6_final_with_diodes"

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
    set flag [antennachecker::checkViolation $net]
    if {$flag == 0} {
      continue
    }
  
    if { [$net isSpecial] } {
      continue
    }
  
    foreach iterm [$net getITerms] {
      set inst [$iterm getInst]
  
      dict set inst_count $inst [expr [dict get $inst_count $inst] + 1]
  
      set count [dict get $inst_count $inst]
      
      set antenna_inst_name "ANTENNA"
      append antenna_inst_name "_" [$inst getName] "_" $count
   
      if {[catch {add_antenna_cell $net $antenna_cell_name $inst $iterm $antenna_inst_name $count} result] } {
        puts "adding node failed"
        continue
      } else {
        set antenna_inst [$block findInst $antenna_inst_name]
        dict set inst_count $antenna_inst 1 
        set antenna_node_counts [expr $antenna_node_counts + 1]
      }
  
      #if {[catch {add_antenna_cell_no_route $db2 $antenna_cell_name $inst $antenna_inst_name $count} result] } {
      #  puts "error occurs"
      #  continue
      #}
      break  
   
    }

    if { $antenna_node_counts == 4 } {
      break
    }
  }
  
  set iterate_times [expr $iterate_times + 1]

}
#foreach inst $insts {
#  puts "error inst name: [$inst getName]"
#}

puts "end reading"

# =================================================
# Finish insertion
# ================================================

set verilog_file_name "$target_file.v"
write_verilog  $verilog_file_name

set def_file_name "$target_file.def"
write_def $def_file_name

#check_antenna

#foreach item [dict keys $inst_count] {
#  set value [dict get $inst_count $item]
#  puts "inst-[$item getName] appears: $value times"
#}

#antenna_node_VDD_bp_processor.cc.y_0__x_0__tile_node.tile.cce.directory.directory.macro_bmem.db1_wb_0__bank.macro_mem
#set nets_count [$db getAllNets]
#puts "# of nets:"
#puts nets_count

#set f [open $viofilename r]
#foreaich net_name [split [read $f] \n] {
#    set net [$block findNet $net_name]
#    set antenna_inst_name "antenna_node_"
#    append antenna_inst_name
#}


#set net_name "clknet_opt_713_clk_C_i_int"
#set net [$block findNet $net_name]


#set antenna_inst_name "antenna_node_0"

#set inst [$block findInst $inst_name]
#add_antenna_cell $net $antenna_cell_name $inst $sink_iterm $antenna_inst_name

#write_lef /afs/eecs.umich.edu/vlsida/projects/openroad/users/wenbo/node_inserted_gms.lef

#set target_def "6_final_all_diodes_inerted_on_side.def"
#set def_write_result [odb::write_def $block $target_def]
#
#if {$def_write_result != 1} {
#  exit 1
#}
#exit 0

# antennachecker::antennachecker_helper
# check_par_max_length -net_name clk -route_level 2
#add_antenna_cell -net hello -antenna_cell_name antenna_cell -sink_inst module1 -sink_iterm gate -antenna_inst_name antenna_inst 
#write_def example1_output.def
