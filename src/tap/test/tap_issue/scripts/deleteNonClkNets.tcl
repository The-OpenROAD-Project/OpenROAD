read_lef $::env(TECH_LEF)
read_lef $::env(SC_LEF)
if {[info exist ::env(ADDITIONAL_LEFS)]} {
  foreach lef $::env(ADDITIONAL_LEFS) {
    read_lef $lef
  }
}

# Read liberty files
foreach libFile $::env(LIB_FILES) {
  read_liberty $libFile
}
# Read def and sdc
read_def $::env(RESULTS_DIR)/6_final.def

set block [[[ord::get_db] getChip] getBlock]
set nets  [$block getNets]
set insts [$block getInsts]

# Delete all non-clock nets
foreach net $nets {
  set sigType [$net getSigType]
  set wire [$net getWire]
  if {"$sigType" eq "SIGNAL" && "$wire" ne "NULL"} {
    odb::dbWire_destroy $wire
  } elseif {"$sigType" eq "POWER" ||
            "$sigType" eq "GROUND"} {
    $net destroySWires
  }
}

# Delete fill cells to clean up screenshot
foreach inst $insts {
  if {"[[$inst getMaster] getType]" eq "CORE_SPACER"} {
    odb::dbInst_destroy $inst
  }
}
write_def $::env(RESULTS_DIR)/6_final_only_clk.def
