// TOP: top
// TECH: nangate45
// TARGETS: tri0, net_type, lexer_gap
// CLUE: tri0/tri1/trireg are legal Verilog-2005 net types but are not lexer keywords,
// CLUE: so "tri0 w;" reduces as ID ID ';' which matches no stmt production -> STA-0171.
module top (a, y);
  input a;
  output y;
  tri0 w;
  BUF_X1 g1 (.A(a), .Z(w));
  INV_X1 g2 (.A(w), .ZN(y));
endmodule
