// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_name_collision, nc2_variant
// CLUE: variant of the _NC1 collision: user wire is named _NC2 (the SECOND
// filler name). Checks whether filler numbering skips taken names at all.
module sub (input a, input [3:0] db, output y);
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  wire _NC2;
  wire d0;
  BUF_X1 u1 (.A(x), .Z(_NC2));
  INV_X1 u2 (.A(_NC2), .ZN(d0));
  sub u0 (.a(x), .y(y));
endmodule
