// TOP: top
// TECH: nangate45
// TARGETS: escaped_brackets, bus_bit_lookalike, port_namespace
// CLUE: an escaped scalar port \y[0] and a real bus port y[1:0] are two distinct
// CLUE: identifiers.  netVerilogName strips a bus subscript while portVerilogName does
// CLUE: not, so the writer has two spellings for what must stay two objects.
module top (a, y, \y[0] );
  input [1:0] a;
  output [1:0] y;
  output \y[0] ;
  INV_X1 g0 (.A(a[0]), .ZN(y[0]));
  INV_X1 g1 (.A(a[1]), .ZN(y[1]));
  BUF_X1 g2 (.A(a[0]), .Z(\y[0] ));
endmodule
