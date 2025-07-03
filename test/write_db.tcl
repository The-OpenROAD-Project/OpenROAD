source "helpers.tcl"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_top.v
link_design mpd_top


set db_file [make_result_file write_db.db]
write_db $db_file
set db_gzip_file [make_result_file write_db.zipped.db.gz]
write_db $db_gzip_file

exec gunzip -f $db_gzip_file
set db0 [exec md5sum $db_file]
set db1 [exec md5sum [string range $db_gzip_file 0 end-3]]

# Remove filename from output
set db0 [string range $db0 0 32]
set db1 [string range $db1 0 32]
if {$db0 == $db1} {
    puts "Matched"
} else {
    puts "Differences found in dbs"
}
