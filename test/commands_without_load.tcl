# Ensure that running commands without loading a design doesn't crash.

set skip {
  cluster_flops
  check_placement
  repair_antennas
  triton_part_design
  optimize_mirroring
  evaluate_part_design_solution
  place_endcaps
  highlight_path
  remove_fillers
  check_antennas
  global_placement_debug
  improve_placement
  pdngen
  insert_dft
  set_driving_cell
  tapcell
  tapcell_ripup
  preview_dft
  define_corners
  exit
}

foreach command [info commands] {
  if {[lsearch $skip $command] != -1} {
    puts "skip $command"
  } else {
    catch {$command} msg
  }
}

# If we got here without crashing then we passed
puts pass
