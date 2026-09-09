// TOP: top
// TECH: nangate45
// TARGETS: nc_filler, scalar_formal, open_pin_record
// CLUE: findPortNCcount is only reached for ports with members, and writeInstPin
// CLUE: prints NOTHING when the pin has no net -- so an open SCALAR formal (input or
// CLUE: output) leaves no ".i()" placeholder at all: the open-pin record is erased.
module leaf (s, i, o, oz);
  input s;
  input i;
  output o;
  output oz;
  INV_X1 g (.A(s), .ZN(o));
  BUF_X1 h (.A(s), .Z(oz));
endmodule

module top (a, y);
  input a;
  output y;
  leaf L (.s(a), .o(y));
endmodule
