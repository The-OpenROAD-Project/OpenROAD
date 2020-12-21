source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def testcase/ispd18_sample/ispd18_sample.input.def
detailed_route -param testcase/ispd18_sample/ispd18_sample.param

set def_file [make_result_file ispd18_sample.outputDB.def]
write_def $def_file
diff_files testcase/ispd18_sample/golden/ispd18_sample.outputDB.defok $def_file
