// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, char_plus, depth_1
// CLUE: escaped instance name with '+'
module top (a, z);
  input a;
  output z;
  wire n1;
  BUF_X1 \i+1 (.A(a), .Z(n1));
  INV_X1 u2 (.A(n1), .ZN(z));
endmodule
