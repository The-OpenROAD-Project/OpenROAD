// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, explicit_empty_named
// CLUE: sub output z connected via explicit-empty .z(). Its internal driver u2
//       is pure dead logic from the parent's view; does it survive both paths?
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
  sub u0 (.a(x), .y(y), .z());
endmodule
