source "helpers.tcl"

proc newdirect { args } {
    set rpt [make_result_file tee_fails_2.rpt]
    tee -file $rpt { puts "HERE" }
}

set rpt [make_result_file tee_fails_1.rpt]

catch { tee -file $rpt { newdirect } } err
puts $err

catch { tee { puts "HERE" } } err
puts $err
