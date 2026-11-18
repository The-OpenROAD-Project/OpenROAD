// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_in_module, name_leak_collision
// CLUE: hier renames a submodule's undriven net to <instpath>/<net>. Here sub
// ALREADY owns a live net literally named \u1/und , so the rename must
// collide with it inside module sub.
module sub (input a, output y);
  wire und;
  wire d;
  wire dd;
  wire \u1/und ;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und), .ZN(d));
  BUF_X1 g3 (.A(a), .Z(\u1/und ));
  INV_X1 g4 (.A(\u1/und ), .ZN(dd));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule
