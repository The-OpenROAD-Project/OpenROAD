// TOP: top
// TECH: nangate45
// TARGETS: bus_direction, per_bit_concat, hier_formal
// CLUE: hier always rewrites a whole-bus formal connection as an explicit per-bit
// CLUE: concat built from memberIterator order (VerilogWriter.cc:410-415).  For an
// CLUE: LSB-FIRST formal [0:1] the leftmost concat element must be bit 0; if the writer
// CLUE: emits MSB-first the two bits are swapped and the logic changes.
module sub (i, o);
  input [0:1] i;
  output [0:1] o;
  INV_X1 g0 (.A(i[0]), .ZN(o[0]));
  BUF_X1 g1 (.A(i[1]), .Z(o[1]));
endmodule

module top (a, z);
  input [0:1] a;
  output [0:1] z;
  sub u (.i(a), .o(z));
endmodule
