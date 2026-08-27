// TOP: top
// TECH: nangate45
// TARGETS: keyword_adjacent, port, depth_1
// CLUE: Keyword-adjacent port names: input$ and inout1 as inputs, output_ as output.
module top (input$, inout1, output_);
  input input$, inout1;
  output output_;
  wire n;
  AND2_X1 u1 (.A1(input$), .A2(inout1), .ZN(n));
  INV_X1 u2 (.A(n), .ZN(output_));
endmodule
