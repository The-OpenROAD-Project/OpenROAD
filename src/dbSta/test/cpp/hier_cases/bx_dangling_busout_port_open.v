// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_output_dangling, explicit_empty_named, nc_filler_for_output
// CLUE: unconnected bus OUTPUT port .q() on a sub whose q bits are all driven
// internally. Does hier invent _NC fillers for outputs too, and do the
// four internal driver gates survive?
module sub (input a, output y, output [3:0] q);
  INV_X1 u1 (.A(a), .ZN(y));
  BUF_X1 g0 (.A(a), .Z(q[0]));
  BUF_X1 g1 (.A(a), .Z(q[1]));
  BUF_X1 g2 (.A(a), .Z(q[2]));
  BUF_X1 g3 (.A(a), .Z(q[3]));
endmodule
module top (input x, output y);
  sub u0 (.a(x), .y(y), .q());
endmodule
