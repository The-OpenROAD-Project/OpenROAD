// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, omitted_named_connection
// CLUE: whole 4-bit sub input bus db omitted from the connection list.
//       Check the [3:0] formal survives in hier output.
module sub (a, db, y);
  input a;
  input [3:0] db;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (.a(x), .y(y));
endmodule
