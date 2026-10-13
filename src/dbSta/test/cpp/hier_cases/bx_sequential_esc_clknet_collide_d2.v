// TOP: top
// TECH: nangate45
// TARGETS: escaped_net_name_equals_depth_2_flattened_clock_path
// CLUE: depth bracket of esc_clknet_collide: the pre-existing top net is named
// \u/c/ckb , matching the two-level flattened path of the leaf's buffered
// clock. Confirms whether the collision depends on path depth.

module leaf (input d, input ck, output q);
  wire ckb;
  CLKBUF_X1 cb (.A(ck), .Z(ckb));
  DFF_X1 ff (.D(d), .CK(ckb), .Q(q));
endmodule

module mid (input d, input ck, output q);
  leaf c (.d(d), .ck(ck), .q(q));
endmodule

module top (input d, input ck, output q, output z);
  wire \u/c/ckb ;
  INV_X1 ci (.A(ck), .ZN(\u/c/ckb ));
  mid u (.d(d), .ck(ck), .q(q));
  DFF_X1 ff2 (.D(d), .CK(\u/c/ckb ), .Q(z));
endmodule
