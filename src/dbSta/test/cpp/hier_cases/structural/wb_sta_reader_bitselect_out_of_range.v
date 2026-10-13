// TOP: top
// TECH: nangate45
// TARGETS: bit_select, range_check, implicit_net
// CLUE: VerilogNetBitSelect just string-formats "w[3]" (VerilogReader.cc:1109) and no
// CLUE: code ever compares the index with the declaration's range, so an out-of-range
// CLUE: bit select silently invents a brand new floating net instead of warning.
module top (a, b, y);
  input a;
  input b;
  output y;
  wire [1:0] w;
  INV_X1 g0 (.A(a), .ZN(w[0]));
  INV_X1 g1 (.A(b), .ZN(w[1]));
  NAND2_X1 g2 (.A1(w[0]), .A2(w[3]), .ZN(y));
endmodule
