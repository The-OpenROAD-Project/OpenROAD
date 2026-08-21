// TOP: top
// TECH: nangate45
// TARGETS: undriven_net_in_module, escaped_undriven_net, name_leak
// CLUE: undriven net inside sub carries an escaped name \und! . If hier stamps
// the instance path onto it the emitted name must stay a legal escaped id.
module sub (input a, output y);
  wire \und! ;
  wire d;
  INV_X1 g1 (.A(a), .ZN(y));
  INV_X1 g2 (.A(\und! ), .ZN(d));
endmodule
module top (input in1, output out1);
  sub u1 (.a(in1), .y(out1));
endmodule
