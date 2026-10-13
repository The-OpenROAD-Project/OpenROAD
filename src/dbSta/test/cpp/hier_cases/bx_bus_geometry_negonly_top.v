// TOP: top
// TECH: nangate45
// TARGETS: neg_bounds, top_ports, depth_0
// CLUE: Bracket: top port range ENTIRELY negative [-5:-2]; every bit should be affected by the escaped-scalar split.
module top (x, z);
  input [-5:-2] x;
  output [-5:-2] z;
  INV_X1 g0 (.A(x[-5]), .ZN(z[-5]));
  INV_X1 g1 (.A(x[-4]), .ZN(z[-4]));
  INV_X1 g2 (.A(x[-3]), .ZN(z[-3]));
  INV_X1 g3 (.A(x[-2]), .ZN(z[-2]));
endmodule
