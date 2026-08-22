// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, omitted_named_connection
// CLUE: sub output z omitted entirely from the named connection list.
//       Internal driver u2 becomes unobservable; check its survival.
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
  sub u0 (.a(x), .y(y));
endmodule
