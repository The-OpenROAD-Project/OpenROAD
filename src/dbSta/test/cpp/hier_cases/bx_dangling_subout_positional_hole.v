// TOP: top
// TECH: nangate45
// TARGETS: sub_output_dangling, positional_empty_slot
// CLUE: positional instantiation sub u0 (x, , y); where the empty middle slot
//       is an OUTPUT formal z. Dangling output via ordered connection.
module sub (a, z, y);
  input a;
  output z;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
  BUF_X1 u2 (.A(a), .Z(z));
endmodule

module top (x, y);
  input x;
  output y;
  sub u0 (x, , y);
endmodule
