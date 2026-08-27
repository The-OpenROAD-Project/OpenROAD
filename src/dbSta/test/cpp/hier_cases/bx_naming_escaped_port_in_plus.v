// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, char_plus, depth_1
// CLUE: escaped top-level input port \p+q
module top (\p+q , z);
  input \p+q ;
  output z;
  INV_X1 u1 (.A(\p+q ), .ZN(z));
endmodule
