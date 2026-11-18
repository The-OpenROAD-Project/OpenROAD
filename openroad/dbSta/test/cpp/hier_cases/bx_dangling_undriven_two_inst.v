// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_in_module, two_instances, name_leak_bracket
// CLUE: bracket for the \u1/und leak: sub (with an undriven-net dead island)
// is instantiated TWICE. If the hier writer stamps an instance path into the
// module-local net name, one instance's path must win (or modules uniquify).
module sub (input a, output y);
  wire und;
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(und), .ZN(d));
endmodule
module top (input in1, input in2, output out1, output out2);
  sub u1 (.a(in1), .y(out1));
  sub u2 (.a(in2), .y(out2));
endmodule
