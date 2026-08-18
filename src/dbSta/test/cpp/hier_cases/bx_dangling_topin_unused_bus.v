// TOP: top
// TECH: nangate45
// TARGETS: top_input_unused, bus_port
// CLUE: 4-bit top input ncb is never referenced. Check the whole bus survives
//       with its declared range (not exploded, not dropped).
module top (x, ncb, y);
  input x;
  input [3:0] ncb;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
endmodule
