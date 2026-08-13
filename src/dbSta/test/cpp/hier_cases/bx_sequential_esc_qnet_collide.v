// TOP: top
// TECH: nangate45
// TARGETS: escaped_net_name_equals_flattened_register_output_path
// CLUE: data-path bracket of esc_clknet_collide: the colliding name is a
// REGISTER OUTPUT net (\u/qi ) instead of a clock net, and the two flops that
// would be shorted have opposite polarity.

module sub (input d, input ck, output z);
  wire qi;
  DFF_X1 ff (.D(d), .CK(ck), .Q(qi));
  INV_X1 i0 (.A(qi), .ZN(z));
endmodule

module top (input d, input ck, output z0, output z1);
  wire \u/qi ;
  DFF_X1 ff2 (.D(d), .CK(ck), .QN(\u/qi ));
  sub u (.d(d), .ck(ck), .z(z0));
  assign z1 = \u/qi ;
endmodule
