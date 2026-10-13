// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, dead_wire_in_parent
// CLUE: sub output z is connected to parent wire zw which drives nothing.
//       Writer may drop zw, the connection, or the driving gate u2 in sub.
module sub (a, y, z);
  input a;
  output y;
  output z;
  INV_X1 u1 (.A(a), .ZN(y));
  BUF_X1 u2 (.A(a), .Z(z));
endmodule

module top (x, y);
  input x;
  output y;
  wire zw;
  sub u0 (.a(x), .y(y), .z(zw));
endmodule
