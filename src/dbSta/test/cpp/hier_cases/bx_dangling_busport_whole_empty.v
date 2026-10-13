// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, explicit_empty_named
// CLUE: whole 4-bit sub input bus db left unconnected via .db(). Bus formal
//       must survive with range; unused inside sub as well.
module sub (a, db, y);
  input a;
  input [3:0] db;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .db(), .y(y));
endmodule
