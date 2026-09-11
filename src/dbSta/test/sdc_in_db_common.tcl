# Shared by the sdc_in_db tests. A test asserts the properties it exists to
# verify with `check` and ends with exit_summary; nothing incidental (info
# lines, record dumps, ids, paths) reaches the log.
source "helpers.tcl"

# "LEF file: ..." and "Restored the timing constraints ...".
suppress_message ODB 227
suppress_message STA 3011

proc load_libs { } {
  read_lef Nangate45/Nangate45.lef
  read_liberty Nangate45/Nangate45_typ.lib
}

proc load_design { netlist } {
  load_libs
  read_verilog $netlist
  link_design top
}

proc native_record { } {
  set prop [odb::dbStringProperty_find [ord::get_db_block] "sta.sdc.native"]
  return [expr { $prop == "NULL" ? "" : [$prop getValue] }]
}

proc has_property { name } {
  return [expr { [odb::dbStringProperty_find [ord::get_db_block] $name] != "NULL" }]
}

proc file_bytes { path } {
  set fh [open $path r]
  fconfigure $fh -translation binary
  set bytes [read $fh]
  close $fh
  return $bytes
}

proc same_file { a b } {
  return [expr { [file_bytes $a] eq [file_bytes $b] }]
}

proc file_matches { path pattern } {
  return [regexp -- $pattern [file_bytes $path]]
}

# Run a second openroad on a script from this directory and return what it
# printed; the child's output is data for `check`, never log.
proc run_child { script } {
  return [exec $::argv0 -no_init -no_splash -exit $script 2>@1]
}

# The kind the child saw after read_db -sdc (see sdc_in_db_restore.tcl).
proc child_kind { output } {
  if { [regexp {stored form: (\w+)} $output -> kind] } {
    return $kind
  }
  return "unreported"
}
