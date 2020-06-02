proc read_libraries {} {
  global tech_lef std_cell_lef extra_lef liberty_file extra_liberty

  read_lef $tech_lef
  read_lef $std_cell_lef
  foreach file $extra_lef { read_lef $file }
  read_liberty $liberty_file
  foreach file $extra_liberty { read_liberty $file }
}

proc have_macros {} {
  set db [::ord::get_db]
  set block [[$db getChip] getBlock]
  foreach inst [$block getInsts] {
    set inst_master [$inst getMaster]
    # BLOCK means MACRO cells
    if { [string match [$inst_master getType] "BLOCK"] } {
      return 1
    }
  }
  return 0
}

proc make_tr_lef {} {
  global design tech_lef std_cell_lef extra_lef
  set merged_lef [make_result_file ${design}_merged.lef]
  set lef_files "$tech_lef $std_cell_lef $extra_lef"
  catch [concat exec mergeLef.py --inputLef $lef_files --outputLef $merged_lef]
  set tr_lef [make_result_file ${design}_tr.lef]
  exec modifyLefSpacing.py -i $merged_lef -o $tr_lef
  return $tr_lef
}

# This is why parameter files suck.
proc make_tr_params { tr_lef cts_def route_guide routed_def } {
  global design
  set tr_params [make_result_file ${design}_tr.params]
  set stream [open $tr_params "w"]
  puts $stream "lef:$tr_lef"
  puts $stream "def:$cts_def"
  puts $stream "guide:$route_guide"
  puts $stream "output:$routed_def"
  puts $stream "outputTA:[make_result_file "${design}_route_TA.def"]"
  puts $stream "outputguide:[make_result_file "${design}_output_guide.mod"]"
  puts $stream "outputDRC:[make_result_file "${design}_route_drc.rpt"]"
  puts $stream "outputMaze:[make_result_file "${design}_maze.log"]"
  puts $stream "threads:1"
  puts $stream "cpxthreads:1"
  puts $stream "verbose:1"
  puts $stream "gap:0"
  puts $stream "timeout:2400"
  close $stream
  return $tr_params
}
