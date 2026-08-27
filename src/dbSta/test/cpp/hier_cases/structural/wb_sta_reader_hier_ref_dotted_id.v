// TOP: top
// TECH: nangate45
// TARGETS: hierarchical_reference, dotted_identifier, implicit_net
// CLUE: VerilogLex.ll:147 lexes {ID_TOKEN}("."{ID_TOKEN})* as ONE identifier, so the
// CLUE: legal hierarchical net reference u1.n becomes a brand new flat net literally
// CLUE: named "u1.n" with no driver and no diagnostic.
module sub (i, o);
  input i;
  output o;
  wire n;
  INV_X1 g0 (.A(i), .ZN(n));
  BUF_X1 g1 (.A(n), .Z(o));
endmodule

module top (a, y, z);
  input a;
  output y;
  output z;
  sub u1 (.i(a), .o(y));
  INV_X1 g2 (.A(u1.n), .ZN(z));
endmodule
