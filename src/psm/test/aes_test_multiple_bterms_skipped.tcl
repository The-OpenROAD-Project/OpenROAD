source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def Nangate45_data/aes_multi_bterms.def
read_liberty Nangate45/Nangate45_typ.lib
read_sdc Nangate45_data/aes.sdc

analyze_power_grid -net VDD

# Disconnect all metal6
set bterm [[ord::get_db_block] findBTerm VDD]
foreach bpin [$bterm getBPins] {
  foreach box [$bpin getBoxes] {
    if { [$box getTechLayer] == [[ord::get_db_tech] findLayer metal6] } {
      odb::dbBoolProperty_create $box PSM_DISCONNECT 1
    }
  }
}

analyze_power_grid -net VDD
