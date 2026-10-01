// TOP: top
// TECH: nangate45
// TARGETS: chain_depth_6, passthrough_wrappers, gated_leaf
// CLUE: 6-level pure wrapper chain, the only gate is at the leaf; stresses
// nested boundary preservation (hier) and deep synthesized instance path
// names (flat).

module leaf (input a, output z);
  INV_X1 g (.A(a), .ZN(z));
endmodule

module c5 (input a, output z);
  leaf u (.a(a), .z(z));
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
