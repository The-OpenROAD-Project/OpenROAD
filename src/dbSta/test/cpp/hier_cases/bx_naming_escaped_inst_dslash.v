// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, double_slash, depth_1
// CLUE: instance \a//b -- unescaped emission turns rest of line into a comment
module top (a, z);
  input a;
  output z;
  wire n1;
  BUF_X1 \a//b (.A(a), .Z(n1));
  INV_X1 u2 (.A(n1), .ZN(z));
endmodule
