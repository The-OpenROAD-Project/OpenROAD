source "helpers.tcl"

proc test_tee { args } {
    puts "FILE TEE: ${args}"
}

proc test_tee_utl { args } {
    utl::info FLW 3 "LOGGER TEE: ${args}"
}

set rpt1 [make_result_file tee_1.rpt]
set rpt2 [make_result_file tee_2.rpt]
set rpt3 [make_result_file tee_3.rpt]
set rpt4 [make_result_file tee_4.rpt]

tee -file $rpt1 { test_tee "FILE1" }
tee -file $rpt2 { test_tee "FILE2" }
tee -append -file $rpt2 { test_tee "FILE3" }

tee -file $rpt3 { test_tee_utl "FILE1" }
tee -file $rpt4 { test_tee_utl "FILE2" }
tee -append -file $rpt4 { test_tee_utl "FILE3" }

diff_file tee_1.rptok $rpt1
diff_file tee_2.rptok $rpt2
diff_file tee_3.rptok $rpt3
diff_file tee_4.rptok $rpt4

set rpt5 [make_result_file tee_5.rpt]
set rpt6 [make_result_file tee_6.rpt]
set rpt7 [make_result_file tee_7.rpt]
set rpt8 [make_result_file tee_8.rpt]

puts "quiet start"
tee -quiet -file $rpt5 { test_tee "FILE4" }
tee -quiet -file $rpt6 { test_tee "FILE5" }
tee -quiet -append -file $rpt6 { test_tee "FILE5" }

tee -quiet -file $rpt7 { test_tee_utl "FILE4" }
tee -quiet -file $rpt8 { test_tee_utl "FILE5" }
tee -quiet -append -file $rpt8 { test_tee_utl "FILE6" }
puts "quiet end"

diff_file tee_5.rptok $rpt5
diff_file tee_6.rptok $rpt6
diff_file tee_7.rptok $rpt7
diff_file tee_8.rptok $rpt8

puts "string redirect start"
tee -variable puts_redirect {puts "PUTS REDIRECT"}
tee -variable logger_redirect {utl::info FLW 4 "LOGGER REDIRECT"}
tee -append -variable logger_redirect_append {utl::info FLW 5 "LOGGER REDIRECT"}
tee -append -variable logger_redirect_append {utl::info FLW 6 "LOGGER REDIRECT"}
puts "string redirect end"

puts "PUTS: $puts_redirect"
puts "LOGGER: $logger_redirect"
puts "LOGGER: $logger_redirect_append"

puts "string redirect start - quiet"
tee -quiet -variable puts_redirect {puts "PUTS REDIRECT"}
tee -quiet -variable logger_redirect {utl::info FLW 7 "LOGGER REDIRECT"}
tee -quiet -append -variable logger_redirect_append {utl::info FLW 8 "LOGGER REDIRECT"}
tee -quiet -append -variable logger_redirect_append {utl::info FLW 9 "LOGGER REDIRECT"}
puts "string redirect end - quiet"

puts "PUTS: $puts_redirect"
puts "LOGGER: $logger_redirect"
puts "LOGGER: $logger_redirect_append"
