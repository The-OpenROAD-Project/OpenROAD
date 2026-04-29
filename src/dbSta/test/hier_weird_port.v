// Reproducer: Verilog escaped identifier containing '/' on a sub-module port.
// After read_verilog + link_design -hier, the modbterm on the sub master is
// stored with '/' as literal characters (backslash-escape preserved).  Before
// the fix in src/odb/src/db/dbModule.cpp, dbModule::findModBTerm truncated
// names at the last '/' before hashing, so lookups of such modbterms missed.
// That miss crashed dbNetwork::direction in downstream graph construction.
// With the fix, pin-direction resolution succeeds.

module sub (\path/with/slash , out);
  input \path/with/slash ;
  output out;
  wire intermediate;
  snl_bufx1 u1 (.A(\path/with/slash ), .Z(intermediate));
  snl_bufx1 u2 (.A(intermediate), .Z(out));
endmodule

module top (in1, out1);
  input in1;
  output out1;
  sub u_sub (.\path/with/slash (in1), .out(out1));
endmodule
