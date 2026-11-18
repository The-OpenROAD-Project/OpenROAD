# Sequential equivalence checking for the retiming tests.
#
# Retiming moves registers, so the netlists before and after are not
# combinationally equivalent; they are checked with ABC `dsec` on AIGER files
# produced by yosys.  Like run_equivalence_test in test/helpers.tcl, the check
# only runs when EQUIVALENCE_CHECK is set (yosys and yosys-abc must be on
# PATH); otherwise it reports the same line as a pass, so one golden log
# serves both cases.  Use it on small designs only: dsec run time grows
# quickly with the number of registers.
#
# Initial state: the registers the retimer creates have no defined power-up
# value, so both netlists are first simulated with the reset asserted for
# `settle` cycles and dsec starts from the state they settle in.  The design
# under test therefore needs a reset that reaches every register.

proc write_verilog_for_dsec { test stage } {
  write_verilog [make_result_file "${test}_$stage.v"]
}

sta::define_cmd_args "run_dsec_test" {test -liberty_files files -clock port \
                                        [-reset port] [-resetn port] \
                                        [-settle cycles]}

proc run_dsec_test { test args } {
  sta::parse_key_args "run_dsec_test" args \
    keys {-liberty_files -clock -reset -resetn -settle} flags {}

  write_verilog_for_dsec $test after

  if { ![info exists ::env(EQUIVALENCE_CHECK)] } {
    puts "Retime output passed/skipped sequential equivalence test"
    return
  }

  set settle 16
  if { [info exists keys(-settle)] } {
    set settle $keys(-settle)
  }
  if { [info exists keys(-resetn)] } {
    set reset_arg "-resetn $keys(-resetn)"
  } else {
    set reset_arg "-reset $keys(-reset)"
  }
  set top [current_design]

  foreach stage {before after} {
    set netlist [make_result_file "${test}_$stage.v"]
    set aig [make_result_file "${test}_$stage.aig"]
    set script [make_result_file "${test}_$stage.ys"]
    set out [open $script w]
    foreach lib $keys(-liberty_files) {
      set lib [file normalize $lib]
      if { [file extension $lib] == ".gz" } {
        # yosys reads plain liberty only.
        set plain [make_result_file "${test}_[file rootname [file tail $lib]]"]
        exec gunzip -c $lib > $plain
        set lib $plain
      }
      puts $out "read_liberty -ignore_miss_func $lib"
    }
    puts $out "read_verilog $netlist"
    puts $out "hierarchy -top $top -check"
    puts $out "flatten"
    puts $out "async2sync"
    puts $out "sim -clock $keys(-clock) $reset_arg -rstlen $settle -n $settle -zinit -w"
    # async2sync leaves synchronous-reset flops, which aigmap does not take.
    puts $out "dffunmap"
    puts $out "aigmap"
    puts $out "write_aiger $aig"
    close $out
    # yosys warns on stderr; only its exit status decides.
    set yosys_log [make_result_file "${test}_$stage.yosys.log"]
    if { [catch { exec yosys -q $script >& $yosys_log }] } {
      puts "Retime output failed sequential equivalence test (yosys failed, see $yosys_log)"
      return
    }
  }

  set before_aig [make_result_file "${test}_before.aig"]
  set after_aig [make_result_file "${test}_after.aig"]
  set abc_log [make_result_file "${test}_dsec.log"]
  catch { exec yosys-abc -c "dsec $before_aig $after_aig" >& $abc_log }
  set log ""
  if { [file exists $abc_log] } {
    set in [open $abc_log r]
    set log [read $in]
    close $in
  }
  if { [string first "Networks are equivalent" $log] >= 0 } {
    puts "Retime output passed/skipped sequential equivalence test"
  } else {
    puts "Retime output failed sequential equivalence test"
  }
}
