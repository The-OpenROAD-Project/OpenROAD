// TOP: top
// TECH: nangate45
// TARGETS: instance_partially_unconnected, sequential_cell
// CLUE: DFF u2 has only CK connected; D, Q, QN all dangle. clk input feeds
//       nothing else, so dropping u2 also strands the clk port.
module top (x, clk, y);
  input x;
  input clk;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
  DFF_X1 u2 (.CK(clk));
endmodule
