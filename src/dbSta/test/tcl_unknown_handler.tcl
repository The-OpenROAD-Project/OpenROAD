# Tcl unknown handler behavior kept by the OpenROAD fork of OpenSTA
# (global sta_unknown, read_sdc does not replace it). Command
# abbreviations, including namespace qualified ones, and unquoted bus
# subscripts work without warnings, and odb handle methods work inside
# read_sdc.
source "helpers.tcl"
read_lef liberty1.lef
read_def hier1.def

# Command abbreviation
report_object_full_n [get_cells b1/r1]

# Namespace qualified command abbreviation
puts [[ord::get_db_bl] getName]
utl::metric_int "tcl_unknown_handler" 1

# Unquoted bus subscript
puts b[2]

# Same from a proc in another namespace
namespace eval tcl_unknown_test {
proc ns_abbrev { } {
  return [[ord::get_db_bl] getName]
}
proc ns_bus { } {
  return b[1]
}
}
puts [tcl_unknown_test::ns_abbrev]
puts [tcl_unknown_test::ns_bus]

# Abbreviations, odb handle methods and bus subscripts inside read_sdc
read_sdc tcl_unknown_handler.sdc
puts [namespace unknown]
