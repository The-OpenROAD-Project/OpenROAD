# Sequential equivalence checking for the retiming tests.
#
# Retiming moves registers, so the netlists before and after are not
# combinationally equivalent; they are checked with ABC `dsec`.  Under Bazel
# ABC is the @abc//:abc_bin data dependency; elsewhere set ABC_EXE, or the
# check reports the same line as a pass, so one golden log serves both cases.
# Use it on small designs only: dsec run time grows quickly with the number
# of registers.
#
# ABC's liberty reader skips sequential cells, so every register and tie
# cell the tests use has a Verilog model below, built from the helper cells.
# A register cell without a model, or any cell ABC reads as a black box,
# fails the check.  Asynchronous controls act at the clock edge, as with
# yosys async2sync.  A negative-edge register is modeled on the same edge as
# the others: it never moves, so both netlists hold it in the same place.
#
# Initial state: the registers the retimer creates have no defined power-up
# value, so each netlist is wrapped to start from zero with the reset
# asserted for 16 cycles and its outputs held at 0 meanwhile.  The
# design under test therefore needs a reset that reaches every register.

set dsec_helper_lib {library (dsec_helpers) {
  cell (DSEC_BUF) { area : 1; pin (A) { direction : input; }
    pin (Y) { direction : output; function : "A"; } }
  cell (DSEC_INV) { area : 1; pin (A) { direction : input; }
    pin (Y) { direction : output; function : "!A"; } }
  cell (DSEC_AND2) { area : 1; pin (A) { direction : input; }
    pin (B) { direction : input; }
    pin (Y) { direction : output; function : "A&B"; } }
  cell (DSEC_MUX2) { area : 1; pin (A0) { direction : input; }
    pin (A1) { direction : input; } pin (S) { direction : input; }
    pin (Y) { direction : output; function : "(A0&!S)|(A1&S)"; } }
}
}

# cell -> {ports body}
set dsec_models {
  dsec_ff {{C D Q} {
 input C, D; output Q; reg Q;
 always @(posedge C) Q <= D;}}
  DFF_X1 {{D CK Q QN} {
 input D, CK; output Q, QN; reg Q;
 always @(posedge CK) Q <= D;
 DSEC_INV i (.A(Q), .Y(QN));}}
  DFF_X2 {{D CK Q QN} {
 input D, CK; output Q, QN; reg Q;
 always @(posedge CK) Q <= D;
 DSEC_INV i (.A(Q), .Y(QN));}}
  DFFR_X1 {{D RN CK Q QN} {
 input D, RN, CK; output Q, QN; wire d; reg s;
 DSEC_AND2 a (.A(D), .B(RN), .Y(d));
 always @(posedge CK) s <= d;
 DSEC_AND2 b (.A(s), .B(RN), .Y(Q));
 DSEC_INV i (.A(Q), .Y(QN));}}
  LOGIC0_X1 {{Z} {
 output Z;
 DSEC_BUF b (.A(1'b0), .Y(Z));}}
  LOGIC1_X1 {{Z} {
 output Z;
 DSEC_BUF b (.A(1'b1), .Y(Z));}}
  sky130_fd_sc_hd__dfxtp_1 {{CLK D Q} {
 input CLK, D; output Q; reg Q;
 always @(posedge CLK) Q <= D;}}
  sky130_fd_sc_hd__edfxtp_1 {{CLK D DE Q} {
 input CLK, D, DE; output Q; wire d; reg Q;
 DSEC_MUX2 m (.A0(Q), .A1(D), .S(DE), .Y(d));
 always @(posedge CLK) Q <= d;}}
  DFFHQx4_ASAP7_75t_R {{CLK D Q} {
 input CLK, D; output Q; reg Q;
 always @(posedge CLK) Q <= D;}}
  DFFLQx4_ASAP7_75t_R {{CLK D Q} {
 input CLK, D; output Q; reg Q;
 always @(posedge CLK) Q <= D;}}
  DFFHQNx1_ASAP7_75t_R {{CLK D QN} {
 input CLK, D; output QN; reg s;
 always @(posedge CLK) s <= D;
 DSEC_INV i (.A(s), .Y(QN));}}
  sg13g2_dfrbp_1 {{CLK D RESET_B Q Q_N} {
 input CLK, D, RESET_B; output Q, Q_N; wire d; reg s;
 DSEC_AND2 a (.A(D), .B(RESET_B), .Y(d));
 always @(posedge CLK) s <= d;
 DSEC_AND2 b (.A(s), .B(RESET_B), .Y(Q));
 DSEC_INV i (.A(Q), .Y(Q_N));}}
  sg13g2_tiehi {{L_HI} {
 output L_HI;
 DSEC_BUF b (.A(1'b1), .Y(L_HI));}}
  sg13g2_tielo {{L_LO} {
 output L_LO;
 DSEC_BUF b (.A(1'b0), .Y(L_LO));}}
}

proc write_verilog_for_dsec { test stage } {
  write_verilog [make_result_file "${test}_$stage.v"]
}

proc dsec_abc_exe { } {
  if { [info exists ::env(ABC_EXE)] } {
    return $::env(ABC_EXE)
  }
  if { [info exists ::env(TEST_SRCDIR)] } {
    return [lindex [glob -nocomplain $::env(TEST_SRCDIR)/abc*/abc_bin] 0]
  }
  return ""
}

# The models of the cells the netlist uses; "" if a register cell has none.
proc dsec_read_file { path } {
  set in [open $path r]
  set text [read $in]
  close $in
  return $text
}

proc dsec_cell_models { netlist } {
  set text [dsec_read_file $netlist]
  set models ""
  set cells [lsort -unique [regexp -all -inline -line {^\s*\S+(?=\s+\S+\s*\()} $text]]
  foreach cell $cells {
    set cell [string trim $cell]
    if { [dict exists $::dsec_models $cell] } {
      lassign [dict get $::dsec_models $cell] ports body
      append models "module $cell ([join $ports {, }]);$body\nendmodule\n"
    } else {
      set master [[ord::get_db] findMaster $cell]
      if { $master != "NULL" && [$master isSequential] } {
        puts "No dsec model for register cell $cell."
        return ""
      }
    }
  }
  return $models
}

# Holds the active-low reset for `settle` cycles from the zero state and the
# outputs at 0 meanwhile.
proc dsec_wrapper { clock resetn settle } {
  set ins {}
  set outs {}
  foreach port [get_ports *] {
    if { [get_property $port direction] == "input" } {
      lappend ins [get_full_name $port]
    } else {
      lappend outs [get_full_name $port]
    }
  }
  set w "module dsec_wrap ([join [concat $ins $outs] {, }]);\n"
  append w " input [join $ins {, }];\n output [join $outs {, }];\n"
  append w " wire dsec_s0;\n DSEC_BUF one (.A(1'b1), .Y(dsec_s0));\n"
  for { set i 1 } { $i <= $settle } { incr i } {
    set prev dsec_s[expr { $i - 1 }]
    append w " wire dsec_s$i;\n"
    append w " dsec_ff f$i (.C($clock), .D($prev), .Q(dsec_s$i));\n"
  }
  set done dsec_s$settle
  append w " wire dsec_reset;\n"
  append w " DSEC_AND2 r (.A($resetn), .B($done), .Y(dsec_reset));\n"
  set conns {}
  foreach port $ins {
    if { $port == $resetn } {
      lappend conns ".$port\(dsec_reset)"
    } else {
      lappend conns ".$port\($port)"
    }
  }
  foreach port $outs {
    append w " wire dsec_o_$port;\n"
    append w " DSEC_AND2 m_$port (.A(dsec_o_$port), .B($done), .Y($port));\n"
    lappend conns ".$port\(dsec_o_$port)"
  }
  append w " [current_design] dut ([join $conns {, }]);\nendmodule\n"
  return $w
}

sta::define_cmd_args "run_dsec_test" {test -liberty_files files -clock port \
                                        -resetn port}

proc run_dsec_test { test args } {
  sta::parse_key_args "run_dsec_test" args \
    keys {-liberty_files -clock -resetn} flags {}

  write_verilog_for_dsec $test after

  set abc [dsec_abc_exe]
  if { $abc == "" } {
    if { [info exists ::env(TEST_SRCDIR)] } {
      puts "Retime output failed sequential equivalence test (no abc_bin)"
    } else {
      puts "Retime output passed/skipped sequential equivalence test"
    }
    return
  }

  set wrapper [dsec_wrapper $keys(-clock) $keys(-resetn) 16]
  set helper_lib [make_result_file "${test}_dsec_helpers.lib"]
  set out [open $helper_lib w]
  puts $out $::dsec_helper_lib
  close $out
  set read_libs "read_lib $helper_lib"
  foreach lib $keys(-liberty_files) {
    append read_libs "; read_lib -m [file normalize $lib]"
  }
  lassign [dict get $::dsec_models dsec_ff] ports body
  set ff_model "module dsec_ff ([join $ports {, }]);$body\nendmodule\n"

  foreach stage {before after} {
    set netlist [make_result_file "${test}_$stage.v"]
    set models [dsec_cell_models $netlist]
    if { $models == "" } {
      puts "Retime output failed sequential equivalence test"
      return
    }
    set wrapped [make_result_file "${test}_${stage}_dsec.v"]
    set out [open $wrapped w]
    # write_verilog leaves the nets of 1'b0 and 1'b1 undeclared.
    set text [string map {(zero_) (1'b0) (one_) (1'b1)} \
      [dsec_read_file $netlist]]
    puts $out "$wrapper$ff_model$models$text"
    close $out
    set aig [make_result_file "${test}_$stage.aig"]
    set log [make_result_file "${test}_$stage.abc.log"]
    file delete -force $aig
    catch {
      exec $abc -c "$read_libs; read -m -c $wrapped; init -z; strash;\
                    write_aiger $aig" >& $log
    }
    if {
      ![regexp {left 0 black boxes} [dsec_read_file $log]]
      || ![file exists $aig]
    } {
      puts "Retime output failed sequential equivalence test (see $log)"
      return
    }
  }

  set before_aig [make_result_file "${test}_before.aig"]
  set after_aig [make_result_file "${test}_after.aig"]
  set abc_log [make_result_file "${test}_dsec.log"]
  catch { exec $abc -c "dsec $before_aig $after_aig" >& $abc_log }
  if { [string first "Networks are equivalent" [dsec_read_file $abc_log]] >= 0 } {
    puts "Retime output passed/skipped sequential equivalence test"
  } else {
    puts "Retime output failed sequential equivalence test"
  }
}
