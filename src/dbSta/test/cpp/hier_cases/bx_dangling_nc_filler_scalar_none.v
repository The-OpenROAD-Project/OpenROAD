// TOP: top
// TECH: nangate45
// TARGETS: sub_input_unconnected, nc_filler_scalar_control
// CLUE: control isolating filler generation to BUS ports: a SCALAR sub input is
// left open while a user wire _NC1 sits on live logic. If no filler is
// invented, no collision can happen for scalar open ports.
module sub (input a, input b, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  wire _NC1;
  BUF_X1 b1 (.A(x), .Z(_NC1));
  INV_X1 g1 (.A(_NC1), .ZN(y));
  sub u0 (.a(x), .b(), .y());
endmodule
