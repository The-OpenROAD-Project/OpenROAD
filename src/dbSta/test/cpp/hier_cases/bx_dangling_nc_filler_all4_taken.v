// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_name_collision, all_names_taken
// CLUE: user already owns live wires _NC1.._NC4 while sub's 4-bit db is left
// open. Every invented filler name is taken, so all four dangling bus bits
// would be re-pointed at live logic.
module sub (input a, input [3:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  wire _NC1;
  wire _NC2;
  wire _NC3;
  wire _NC4;
  BUF_X1 b1 (.A(x), .Z(_NC1));
  BUF_X1 b2 (.A(x), .Z(_NC2));
  BUF_X1 b3 (.A(x), .Z(_NC3));
  BUF_X1 b4 (.A(x), .Z(_NC4));
  AND2_X1 g1 (.A1(_NC1), .A2(_NC2), .ZN(y));
  sub u0 (.a(_NC3), .y(_NC4));
endmodule
