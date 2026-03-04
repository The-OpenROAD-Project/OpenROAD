# Remove unused master cells from an ODB file.
#
# Environment variables:
#   WHITTLE_DB_INPUT   - path to the input ODB file
#   WHITTLE_DB_OUTPUT  - path to write the cleaned ODB file
#   WHITTLE_DEF_OUTPUT - (optional) path to write DEF output
#
# Prints to stdout:
#   REMOVED <count>

read_db $env(WHITTLE_DB_INPUT)

set removed [[ord::get_db] removeUnusedMasters]
puts "REMOVED $removed"

write_db $env(WHITTLE_DB_OUTPUT)

if { [info exists env(WHITTLE_DEF_OUTPUT)] } {
  odb::write_def [ord::get_db_block] $env(WHITTLE_DEF_OUTPUT)
}

exit
