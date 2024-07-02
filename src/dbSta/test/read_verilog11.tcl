# Read src attributes from Verilog

source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog hier1.v
link_design top

set block [ord::get_db_block]
foreach i [$block getInsts] {
  set src_file_line [odb::dbIntProperty_find $i src_file_line]
  if { $src_file_line !=  "NULL" } {
    set src_file_id [[odb::dbIntProperty_find $i src_file_id] getValue]
    set src_file_str "src_file_$src_file_id"
    set src_file_name [odb::dbStringProperty_find $block $src_file_str]

    puts "[$i getName] [$src_file_name getValue]:[$src_file_line getValue]"
  }
}
