


proc add_antenna_cell { net antenna_cell_name sink_inst sink_iterm antenna_inst_name count} {
  
  set antenna_cell_width [expr 1.512 * 2000]
  set antenna_cell_height [expr 0.576 * 2000]
  
  set block [[[::ord::get_db] getChip] getBlock]
  set net_name [$net getName]
  
  set antenna_master [[::ord::get_db] findMaster $antenna_cell_name]
  set antenna_mterm [$antenna_master getMTerms]

  set inst_loc [$sink_inst getLocation]
  set inst_loc_x [lindex [$sink_inst getLocation] 0]
  set inst_loc_y [lindex [$sink_inst getLocation] 1]
  set diode_loc_x [expr {int($inst_loc_x - $antenna_cell_width - 300)}]
  set diode_loc_y [expr {int($inst_loc_y + ($count - 1) * ($antenna_cell_height + 100))}]
  set inst_ori [$sink_inst getOrient]

  set antenna_inst [odb::dbInst_create $block $antenna_master $antenna_inst_name]
  set antenna_iterm [$antenna_inst findITerm "A"]

  $antenna_inst setLocation $diode_loc_x $diode_loc_y
  $antenna_inst setOrient $inst_ori
  $antenna_inst setPlacementStatus PLACED
  odb::dbITerm_connect $antenna_iterm $net

} 

proc add_antenna_cell_no_route { db antenna_cell_name sink_inst sink_iterm antenna_inst_name count} {
  set antenna_cell_width [expr 1.512 * 2000]
  set antenna_cell_height [expr 0.576 * 2000]

  set block [[$db getChip] getBlock]
  set antenna_master [$db findMaster $antenna_cell_name]

  set inst_loc [$sink_inst getLocation]
  set inst_loc_x [lindex [$sink_inst getLocation] 0]
  set inst_loc_y [lindex [$sink_inst getLocation] 1]
  set diode_loc_x [expr {int($inst_loc_x - $antenna_cell_width - 300)}]
  set diode_loc_y [expr {int($inst_loc_y + ($count - 1) * ($antenna_cell_height + 100))}]
  set inst_ori [$sink_inst getOrient]
  
  puts $diode_loc_x
  puts $diode_loc_y
  set antenna_inst [odb::dbInst_create $block $antenna_master $antenna_inst_name]
  set antenna_iterm [$antenna_inst findITerm "A"]

  $antenna_inst setLocation $diode_loc_x $diode_loc_y
  $antenna_inst setOrient $inst_ori
  $antenna_inst setPlacementStatus PLACED

}
   
