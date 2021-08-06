proc deleteRoutingObstructions {} {
  set db [ord::get_db]
  set chip [$db getChip]
  set block [$chip getBlock]
  set obstructions [$block getObstructions]

  foreach obstruction $obstructions {
    odb::dbObstruction_destroy $obstruction
  }
  puts "\[INFO\] Deleted [llength $obstructions] routing obstructions"
}
