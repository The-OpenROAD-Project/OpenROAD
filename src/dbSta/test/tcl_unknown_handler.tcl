# Tcl unknown handler behavior kept by the OpenROAD fork of OpenSTA
# (global sta_unknown, read_sdc does not replace it). Command
# abbreviations, including namespace qualified ones, and unquoted bus
# subscripts work without warnings, and odb handle methods work inside
# read_sdc.
source "helpers.tcl"
read_lef liberty1.lef
read_def hier1.def

proc check { label script } {
  if { [catch { uplevel #0 $script } result] } {
    puts "$label: error: $result"
  } else {
    puts "$label: $result"
  }
}

namespace eval tcl_unknown_test {
  proc ns_abbrev {} {
    return [[ord::get_db_bl] getName]
  }
  proc ns_bus {} {
    return b[1]
  }
}

check "abbrev" { report_object_full_n [get_cells b1/r1] }
check "ns abbrev" { [ord::get_db_bl] getName }
check "ns abbrev metric" { utl::metric_int "tcl_unknown_handler" 1 }
check "ns abbrev in ns proc" { tcl_unknown_test::ns_abbrev }
check "bus" { set bus b[2] }
check "bus in ns proc" { tcl_unknown_test::ns_bus }
check "read_sdc" { read_sdc tcl_unknown_handler.sdc }
check "handler after read_sdc" { namespace unknown }
