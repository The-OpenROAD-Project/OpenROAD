source "helpers.tcl"

set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design58.def"

set out_def [make_result_file "write_def58_gzip.def"]
write_def $out_def

set out_gzip_def [make_result_file "write_def58_gzip.zipped.def.gz"]
write_def $out_gzip_def

exec gunzip -f $out_gzip_def
set def0 [exec md5sum $out_def]
set def1 [exec md5sum [string range $out_gzip_def 0 end-3]]

# Remove filename from output
set def0 [string range $def0 0 32]
set def1 [string range $def1 0 32]
if { $def0 == $def1 } {
  puts "Matched"
  puts "pass"
} else {
  puts "Differences found in dbs $def0 != $def1"
  puts "failed"
}

exit 0
