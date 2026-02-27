source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design58.def"

set out_def [make_result_file "write_def58_gzip.def"]
write_def $out_def

set out_gzip_def [make_result_file "write_def58_gzip.zipped.def.gz"]
write_def $out_gzip_def

exec gunzip -f $out_gzip_def
set unzipped_def [string range $out_gzip_def 0 end-3]

set f0 [open $out_def r]
set def0 [read $f0]
close $f0

set f1 [open $unzipped_def r]
set def1 [read $f1]
close $f1

if { $def0 == $def1 } {
  puts "Matched"
  puts "pass"
} else {
  puts "Differences found in dbs $def0 != $def1"
  puts "failed"
}

exit 0
