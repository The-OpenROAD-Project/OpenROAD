source "helpers.tcl"

sta::proc_redirect test_redirect {
    puts -nonewline "FILE REDIRECT: ${args}"
}

set rpt1 [make_result_file logger_redirection_nonewline_1.rpt]
set rpt2 [make_result_file logger_redirection_nonewline_2.rpt]

sta::test_redirect "FILE1" > $rpt1
sta::test_redirect "FILE2" > $rpt2
sta::test_redirect "FILE3" >> $rpt2

diff_file logger_redirection_nonewline_1.rptok $rpt1
diff_file logger_redirection_nonewline_2.rptok $rpt2

puts "string redirect start"
with_output_to_variable puts_redirect {puts -nonewline "PUTS REDIRECT"}
puts "string redirect end"

puts "PUTS: $puts_redirect"
