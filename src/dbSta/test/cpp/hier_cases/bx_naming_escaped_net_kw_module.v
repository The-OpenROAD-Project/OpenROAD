// TOP: top
// TECH: nangate45
// TARGETS: escaped_net, keyword_module, depth_1
// CLUE: net named \module ; if the writer normalizes escaped-but-wordlike
// names it emits the bare keyword and the output no longer parses.
module top (input a, output z);
  wire \module ;
  INV_X1 u1 (.A(a), .ZN(\module ));
  INV_X1 u2 (.A(\module ), .ZN(z));
endmodule
