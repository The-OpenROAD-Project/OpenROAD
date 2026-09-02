// TARGETS: nc_filler, sub_bus_input_unconnected, two_digit_index, hier
// CLUE: the filler names are formatted with a decimal counter
// (VerilogWriter.cc:312 and :430), so the tenth one is _NC10.  A 10-bit open
// formal takes the whole one-digit range plus the first two-digit name, and the
// only user net in the design is the live wire _NC10 -- the boundary case the
// _NC1.._NC4 cases cannot reach.
module ncbig (input a, input [9:0] db, output y);
  INV_X1 g (.A(a), .ZN(y));
endmodule

module top (input x, output y, output y2);
  wire _NC10;
  BUF_X1 b1 (.A(x), .Z(_NC10));
  INV_X1 b2 (.A(_NC10), .ZN(y2));
  ncbig u0 (.a(x), .db(), .y(y));
endmodule
