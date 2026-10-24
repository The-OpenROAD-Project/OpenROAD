// TOP: top
// TECH: nangate45
// TARGETS: module_port_never_connected, depth_2
// CLUE: leaf (instantiated at depth 2) has input nc that mid never connects.
//       Unconnected formal two levels down; does it survive hier write?
module leaf (a, nc, y);
  input a;
  input nc;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module mid (a, y);
  input a;
  output y;
  wire t;
  INV_X1 u1 (.A(a), .ZN(t));
  leaf u2 (.a(t), .y(y));
endmodule

module top (x, y);
  input x;
  output y;
  mid u0 (.a(x), .y(y));
endmodule
