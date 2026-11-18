// TOP: top
// TECH: nangate45
// TARGETS: escaped_busbit_lookalike, no_real_bus, depth_1
// CLUE: scalar net named \data[0]  with NO bus named data anywhere; if the
// writer drops the escape, data[0] becomes a select of a nonexistent bus.
module top (input a, output z);
  wire \data[0] ;
  INV_X1 u1 (.A(a), .ZN(\data[0] ));
  INV_X1 u2 (.A(\data[0] ), .ZN(z));
endmodule
