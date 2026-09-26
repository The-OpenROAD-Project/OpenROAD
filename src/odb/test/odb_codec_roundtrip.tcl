# write_db hands the file it wrote, and where its table slots lie, to
# ODB_CODEC, and read_db reads through it: a database written through a
# codec and read back is the database. Without ODB_CODEC, write_db
# writes the database and nothing else.
source "helpers.tcl"

proc check { cond msg } {
  if { ![uplevel 1 [list expr $cond]] } {
    puts "fail: $msg"
    exit 1
  }
}

proc read_bytes { path } {
  set f [open $path rb]
  set bytes [read $f]
  close $f
  return $bytes
}

set codec [file normalize "odb_codec_stub.sh"]
read_db "data/design.odb"

set plain [make_result_file "odb_codec_roundtrip_plain.odb"]
write_db $plain
check { ![file exists $plain.layout] } "write_db wrote a layout without ODB_CODEC"

set ::env(ODB_CODEC) $codec
set encoded [make_result_file "odb_codec_roundtrip.odb"]
write_db $encoded
set f [open $encoded rb]
set first [gets $f]
close $f
check { $first == "odb-codec-stub" } "write_db did not run ODB_CODEC"
check { ![file exists $encoded.layout] } "write_db left its layout behind"

# A second openroad reads the encoded file through the codec and writes
# what it read, without one.
set script [make_result_file "odb_codec_roundtrip_read.tcl"]
set back [make_result_file "odb_codec_roundtrip_back.odb"]
set f [open $script w]
puts $f "read_db $encoded"
puts $f "unset ::env(ODB_CODEC)"
puts $f "write_db $back"
close $f
exec [info nameofexecutable] -no_init -no_splash -exit $script >@ stdout 2>@ stderr
check { [read_bytes $back] eq [read_bytes $plain] } \
  "the database read through the codec differs from the one written"

puts pass
