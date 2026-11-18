// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, dead_assign_load
// CLUE: sub output z -> wire zw -> assign deadw = zw; deadw feeds nothing.
//       The only "load" of the sub output is itself dead.
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
  wire deadw;
  sub u0 (.a(x), .y(y), .z(zw));
  assign deadw = zw;
endmodule
