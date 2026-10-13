// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_vs_user_bus
// CLUE: user wire is a BUS named _NC1[1:0] carrying live logic while sub's db is
// unconnected. A scalar filler named _NC1 collides with a bus of that name.
module sub (input a, input [3:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, input x2, output y);
  wire [1:0] _NC1;
  sub u0 (.a(x), .y(_NC1[0]));
  BUF_X1 b1 (.A(x2), .Z(_NC1[1]));
  AND2_X1 g1 (.A1(_NC1[0]), .A2(_NC1[1]), .ZN(y));
endmodule
