// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, slash_path_collision, escaped_vs_escaped, depth_2
// CLUE: top net \u1/w+w  collides with the flattened name of ESCAPED net
// \w+w  inside submodule instance u1 -- both colliders are escaped ids.
module subw (input a, output z);
  wire \w+w ;
  INV_X1 g1 (.A(a), .ZN(\w+w ));
  INV_X1 g2 (.A(\w+w ), .ZN(z));
endmodule
module top (input a, output z, output w);
  wire \u1/w+w ;
  subw u1 (.a(a), .z(w));
  BUF_X1 g1 (.A(a), .Z(\u1/w+w ));
  BUF_X1 g2 (.A(\u1/w+w ), .Z(z));
endmodule
