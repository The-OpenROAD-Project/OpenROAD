// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, keyword_input, depth_1
// CLUE: top-level port named \input ; the port list would read
// "input input" if the escape is lost.
module top (input \input , output z);
  INV_X1 u1 (.A(\input ), .ZN(z));
endmodule
