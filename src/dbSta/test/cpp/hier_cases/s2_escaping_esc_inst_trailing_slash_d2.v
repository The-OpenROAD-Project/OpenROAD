// TARGETS: escaped_instance, naming_slash, trailing_slash, depth_2
// CLUE: an instance whose escaped name ENDS with the hierarchy divider. Sta name
// is "a\/", so the flat leaf name from dbReadVerilog.cc:538 is "a\//g1" -- TWO
// consecutive '/' where the first is escaped and the second is the real
// divider. dbNetwork::stripParentPrefix scans right to left and steps back one
// char on a preceding backslash (dbNetwork.cc:1401-1408), while
// dbBlock::getBaseName scans left to right on backslash-run parity
// (dbBlock.cpp:3974-3980); the two must agree on which of the adjacent slashes
// is the divider. The corpus covers a LEADING slash (`\/g `) but never a
// trailing one.
module sub (i, o);
  input i;
  output o;
  INV_X1 g1 (.A(i), .ZN(o));
endmodule

module top (a, z);
  input a;
  output z;
  sub \a/  (.i(a), .o(z));
endmodule
