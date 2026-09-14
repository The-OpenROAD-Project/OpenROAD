// TOP: top
// TECH: nangate45
// TARGETS: open_pin, module_formal, scalar_output
// CLUE: makeInstPin (VerilogReader.cc:1711-1729) only creates the child-side net/term
// CLUE: when the parent net exists, so an open SCALAR formal leaves the child driver
// CLUE: with no boundary object at all (the known _NC filler defect covers vectors only).
module sub (i, o, p);
  input i;
  output o;
  output p;
  INV_X1 g0 (.A(i), .ZN(o));
  BUF_X1 g1 (.A(i), .Z(p));
endmodule

module top (a, y);
  input a;
  output y;
  sub u (.i(a), .o(y), .p());
endmodule
