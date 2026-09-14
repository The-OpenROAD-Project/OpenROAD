# report_cell_usage with values too large for the default column widths.
# The count column defaults to 7 characters and the area column to 10, so
# instantiate enough cells to push both past those defaults: >100k instances
# and >10,000,000 um^2 of area.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_lef "Nangate45/fakeram45_1024x32.lef"
read_liberty "Nangate45/Nangate45_typ.lib"
read_def "report_cell_usage.def"

set db [ord::get_db]
set block [ord::get_db_block]

# Widen the count column: 120000 buffers on top of the 676 cells in the design.
set buffer [$db findMaster "BUF_X1"]
for { set i 0 } { $i < 120000 } { incr i } {
  odb::dbInst_create $block $buffer "wide_buf_$i"
}

# Widen the area column: each macro is 104.5 x 317.8 = 33210.1 um^2.
set macro [$db findMaster "fakeram45_1024x32"]
for { set i 0 } { $i < 400 } { incr i } {
  odb::dbInst_create $block $macro "wide_macro_$i"
}

report_cell_usage -verbose
