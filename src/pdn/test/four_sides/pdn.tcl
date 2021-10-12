  read_lef four_sides/tech.lef
  read_lef four_sides/pad.lef
  read_def four_sides/floorplan.def

  pdngen four_sides/pdn.cfg -verbose
  write_def four_sides/pdn.def
