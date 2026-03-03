# Remove unused master cells from an ODB file.
#
# Environment variables:
#   WHITTLE_DB_INPUT   - path to the input ODB file
#   WHITTLE_DB_OUTPUT  - path to write the cleaned ODB file
#   WHITTLE_DEF_OUTPUT - (optional) path to write DEF output
#
# Prints to stdout:
#   REMOVED <count>

set db [odb::dbDatabase_create]
odb::read_db $db $env(WHITTLE_DB_INPUT)

set removed [$db removeUnusedMasters]
puts "REMOVED $removed"

odb::write_db $db $env(WHITTLE_DB_OUTPUT)

if { [info exists env(WHITTLE_DEF_OUTPUT)] } {
  set block [[$db getChip] getBlock]
  odb::write_def $block $env(WHITTLE_DEF_OUTPUT)
}

exit
