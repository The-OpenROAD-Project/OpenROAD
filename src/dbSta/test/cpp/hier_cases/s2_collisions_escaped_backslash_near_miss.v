// TARGETS: flat_path_near_miss, escaped_net, literal_backslash, depth_2
// CLUE: near-miss control for the whole flat-path family.  The top net is the
// identifier x\/y -- a LITERAL backslash followed by a slash -- whose sta form
// is x\\\/y (VerilogNamespace.cc:205-215 escapes both).  staToVerilog collapses
// the doubled escape but keeps one backslash (VerilogNamespace.cc:88-97), so it
// must print \x\/y while the flattened net of instance x prints \x/y .  If these
// two ever merge, the escape handling has over-collapsed; if they stay distinct,
// this guards the exact boundary where the real collisions begin.
module subxy (input a, output z);
  wire y;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(y), .ZN(z));
endmodule

module top (input i1, input i2, output o1, output o2);
  wire \x\/y ;
  subxy x (.a(i1), .z(o1));
  INV_X1 g3 (.A(i2), .ZN(\x\/y ));
  BUF_X1 g4 (.A(\x\/y ), .Z(o2));
endmodule
