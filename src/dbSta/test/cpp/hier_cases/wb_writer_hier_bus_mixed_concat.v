// TOP: top
// TECH: nangate45
// TARGETS: per_bit_concat, hier_formal, mixed_sources, escaped_net_in_concat
// CLUE: hier rewrites every vector formal as an explicit concat built one bit at a time
// CLUE: (writeInstBusPinBit, VerilogWriter.cc:421-437), each bit spelled with
// CLUE: netVerilogName.  Feed the two bits from unrelated sources -- an escaped
// CLUE: slash-bearing scalar and a bit of a different bus -- and make the two child
// CLUE: outputs asymmetric so any bit swap or misspelling is LEC-visible.
module sub (i, o0, o1);
  input [1:0] i;
  output o0;
  output o1;
  INV_X1 g0 (.A(i[0]), .ZN(o0));
  BUF_X1 g1 (.A(i[1]), .Z(o1));
endmodule

module top (a, b, z0, z1);
  input a;
  input [1:0] b;
  output z0;
  output z1;
  wire \x/y ;
  INV_X1 p (.A(a), .ZN(\x/y ));
  sub u (.i({\x/y , b[1]}), .o0(z0), .o1(z1));
endmodule
