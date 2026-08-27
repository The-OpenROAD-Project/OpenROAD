// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, char_slash, depth_2
// CLUE: net named \y/z  INSIDE a submodule; the flattened name u1/y/z
// looks like a depth-3 path though the real depth is 2.
module subs (input a, output z);
  wire \y/z ;
  INV_X1 g1 (.A(a), .ZN(\y/z ));
  INV_X1 g2 (.A(\y/z ), .ZN(z));
endmodule
module top (input a, output z);
  subs u1 (.a(a), .z(z));
endmodule
