// TOP: top
// TECH: nangate45
// TARGETS: chain_depth_10, passthrough_wrappers, gated_leaf
// CLUE: 10-level wrapper chain, gate only at the leaf; deepest chain variant,
// stresses recursion depth of the hier writer and path-name synthesis (flat).

module leaf (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module c9 (input a, output z);
  leaf u (.a(a), .z(z));
endmodule

module c8 (input a, output z);
  c9 u (.a(a), .z(z));
endmodule

module c7 (input a, output z);
  c8 u (.a(a), .z(z));
endmodule

module c6 (input a, output z);
  c7 u (.a(a), .z(z));
endmodule

module c5 (input a, output z);
  c6 u (.a(a), .z(z));
endmodule

module c4 (input a, output z);
  c5 u (.a(a), .z(z));
endmodule

module c3 (input a, output z);
  c4 u (.a(a), .z(z));
endmodule

module c2 (input a, output z);
  c3 u (.a(a), .z(z));
endmodule

module c1 (input a, output z);
  c2 u (.a(a), .z(z));
endmodule

module top (input a, output z);
  c1 u (.a(a), .z(z));
endmodule
