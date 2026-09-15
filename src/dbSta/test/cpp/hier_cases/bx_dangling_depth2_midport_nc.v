// TOP: top
// TECH: nangate45
// TARGETS: module_port_never_connected, mid_level
// CLUE: mid has input nc that top never connects; mid itself contains a leaf
//       at depth 2, so the dangling formal sits on an intermediate module.
module leaf (a, y);
  input a;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module mid (a, nc, y);
  input a;
  input nc;
  output y;
  leaf u2 (.a(a), .y(y));
endmodule

module top (x, y);
  input x;
  output y;
  mid u0 (.a(x), .y(y));
endmodule
