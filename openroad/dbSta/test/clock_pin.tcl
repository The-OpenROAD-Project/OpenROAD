# Set clock pin from Liberty when LEF is missing it
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog clock_pin.v
link_design top

set type [get_property [get_lib_pins NangateOpenCellLibrary/DFFRS_X1/CK] is_register_clock]
puts "STA is_register_clock: $type"

set db [ord::get_db]
set master [$db findMaster DFFRS_X1]
set mterm [$master findMTerm CK]
set type [$mterm getSigType]
puts "ODB type: $type"
