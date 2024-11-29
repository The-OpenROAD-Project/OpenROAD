source "helpers.tcl"

sta::proc_redirect test_redirect {
    puts "FILE REDIRECT: ${args}"
}

sta::proc_redirect test_redirect_utl {
    utl::info FLW 1 "LOGGER REDIRECT: ${args}"
}

set rpt1 [make_result_file logger_redirection_1.rpt]
set rpt2 [make_result_file logger_redirection_2.rpt]
set rpt3 [make_result_file logger_redirection_3.rpt]
set rpt4 [make_result_file logger_redirection_4.rpt]

sta::test_redirect "FILE1" > $rpt1
sta::test_redirect "FILE2" > $rpt2
sta::test_redirect "FILE3" >> $rpt2

sta::test_redirect_utl "FILE1" > $rpt3
sta::test_redirect_utl "FILE2" > $rpt4
sta::test_redirect_utl "FILE3" >> $rpt4

diff_file logger_redirection_1.rptok $rpt1
diff_file logger_redirection_2.rptok $rpt2
diff_file logger_redirection_3.rptok $rpt3
diff_file logger_redirection_4.rptok $rpt4

puts "string redirect start"
with_output_to_variable puts_redirect {puts "PUTS REDIRECT"}
with_output_to_variable logger_redirect {utl::info FLW 2 "LOGGER REDIRECT"}
puts "string redirect end"

puts "PUTS: $puts_redirect"
puts "LOGGER: $logger_redirect"
