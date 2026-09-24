# read_db reports a file that decodes to something other than a database
# as the error it gives without ODB_CODEC, and openroad does not abort.
source "helpers.tcl"

set ::env(ODB_CODEC) [file normalize "odb_codec_stub.sh"]
set bogus [make_result_file "odb_codec_invalid.odb"]
set f [open $bogus wb]
puts -nonewline $f [string repeat "not a database " 10000]
close $f

if { ![catch { read_db $bogus } err] } {
  puts "fail: read_db accepted a file that is not a database"
  exit 1
}
if { ![string match "*not an OpenDB Database*" $err] } {
  puts "fail: unexpected error: $err"
  exit 1
}

puts pass
