set test_case gen_model_3
set gold GOLD
set d /home/dimitris-ic/icb/icbrcx/src/rcx/test/via_res/corners.0322/data.0102

set lef $d/ua11lscef15bdrll_6m2t.lef.$gold
set def $d/umc110.1231.patterns.DEF.$gold

set spef1 $d/SPEF/umc110.1231.patterns.Cmax_125C.spef
set spef2 $d/SPEF/umc110.1231.patterns.Typ_25C.spef
set spef3 $d/SPEF/umc110.1231.patterns.Cmin_125C.spef

read_lef $lef
read_def $def

gen_rcx_model -out_file $test_case.rcx.model -corner_list "MAX TYP MIN" -spef_file_list "$spef1 $spef2 $spef3"

exit
