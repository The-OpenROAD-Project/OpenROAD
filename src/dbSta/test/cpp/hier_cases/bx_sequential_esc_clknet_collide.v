// TOP: top
// TECH: nangate45
// TARGETS: escaped_net_name_equals_flattened_clock_net_path, clock_net_collision
// CLUE: top already owns a net literally named \u/ckb , which is exactly the
// name flattening will synthesise for submodule u's internal buffered clock.
// The two nets have OPPOSITE polarity, so a name-driven merge is observable.

module sub (input d, input ck, output q);
  wire ckb;
  CLKBUF_X1 cb (.A(ck), .Z(ckb));
  DFF_X1 ff (.D(d), .CK(ckb), .Q(q));
endmodule

module top (input d, input ck, output q, output z);
  wire \u/ckb ;
  INV_X1 ci (.A(ck), .ZN(\u/ckb ));
  sub u (.d(d), .ck(ck), .q(q));
  DFF_X1 ff2 (.D(d), .CK(\u/ckb ), .Q(z));
endmodule
