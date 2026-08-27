// TOP: top
// TECH: nangate45
// TARGETS: needs_escape, leading_digit, net_name
// CLUE: staToVerilog2 (VerilogNamespace.cc:147-152) sets escaped only when a char is
// CLUE: not alnum/underscore -- it never checks that the FIRST char is a digit, so an
// CLUE: escaped net name made only of alnum chars is re-emitted BARE as "wire 1w;".
module top (a, y);
  input a;
  output y;
  wire \1w ;
  INV_X1 g1 (.A(a), .ZN(\1w ));
  BUF_X1 g2 (.A(\1w ), .Z(y));
endmodule
