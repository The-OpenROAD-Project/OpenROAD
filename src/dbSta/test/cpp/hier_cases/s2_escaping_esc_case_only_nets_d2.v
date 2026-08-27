// TARGETS: escaped_net, case_sensitivity, depth_2
// CLUE: the corpus tests case-only differences on PLAIN names only
// (bx_naming_plain_case_nets_d1/d3). Two ESCAPED names differing only in case
// take a different route: staToVerilog2 (VerilogNamespace.cc:120) rebuilds the
// string char by char and dbNetwork's lookups key on it after the flat join in
// dbReadVerilog.cc:538 makes "u1/n+A" and "u1/n+a". Any case-folding anywhere
// merges the two cones.
module sub (i0, i1, o0, o1);
  input i0;
  input i1;
  output o0;
  output o1;
  wire \n+A ;
  wire \n+a ;
  INV_X1 g1 (.A(i0), .ZN(\n+A ));
  BUF_X1 g2 (.A(i1), .Z(\n+a ));
  BUF_X1 g3 (.A(\n+A ), .Z(o0));
  INV_X1 g4 (.A(\n+a ), .ZN(o1));
endmodule

module top (a, b, y0, y1);
  input a;
  input b;
  output y0;
  output y1;
  sub u1 (.i0(a), .i1(b), .o0(y0), .o1(y1));
endmodule
