// TOP: top
// TECH: nangate45
// TARGETS: module_order, hier, findHierChildren_sort
// CLUE: findHierChildren sorts the collected children by CELL NAME
// CLUE: (VerilogWriter.cc:145-150), so module definitions are emitted ASCII-ordered
// CLUE: rather than in declaration or instantiation order.
module z_mod (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule

module m_mod (i, o);
  input i;
  output o;
  BUF_X1 g (.A(i), .Z(o));
endmodule

module a_mod (i, o);
  input i;
  output o;
  INV_X1 g (.A(i), .ZN(o));
endmodule

module top (a, y);
  input a;
  output y;
  wire n1;
  wire n2;
  z_mod uz (.i(a), .o(n1));
  m_mod um (.i(n1), .o(n2));
  a_mod ua (.i(n2), .o(y));
endmodule
