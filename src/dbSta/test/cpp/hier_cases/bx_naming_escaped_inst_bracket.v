// TOP: top
// TECH: nangate45
// TARGETS: escaped_inst, bracket, depth_1
// CLUE: instance \g[2] -- looks like an instance-array element
module top (a, z);
  input a;
  output z;
  wire n1;
  BUF_X1 \g[2] (.A(a), .Z(n1));
  INV_X1 u2 (.A(n1), .ZN(z));
endmodule
