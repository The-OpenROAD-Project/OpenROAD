// TOP: top
// TECH: nangate45
// TARGETS: hier, stripParentPrefix, trailing_backslash_guard_misfire
// CLUE: dbNetwork::stripParentPrefix (dbNetwork.cc:791-804) treats a '/' preceded
// by '\' as a literal name char. verilogToSta DOUBLES a trailing backslash, so an
// instance named `\u1\ ` becomes sta "u1\\" and the flat leaf name "u1\\/g1" has a
// backslash immediately before the REAL hierarchy divider: the guard misfires, no
// prefix is stripped, and every leaf/net inside the module should be emitted with
// the whole instance path baked into its name.
module top (a, y);
   input a;
   output y;
   m \u1\  (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
endmodule
