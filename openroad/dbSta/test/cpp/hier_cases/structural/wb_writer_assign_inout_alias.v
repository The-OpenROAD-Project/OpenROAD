// TOP: top
// TECH: nangate45
// TARGETS: assign_emission, inout_port, isAnyOutput
// CLUE: writeAssigns' guard is direction->isAnyOutput(), which is TRUE for bidirect, so
// CLUE: an aliased INOUT top port acquires a continuous assign driving it from inside --
// CLUE: a bidirectional pin turned into a permanently driven one.
module top (a, io, y);
  input a;
  inout io;
  output y;
  wire n;
  INV_X1 g1 (.A(a), .ZN(n));
  assign io = n;
  BUF_X1 g2 (.A(io), .Z(y));
endmodule
