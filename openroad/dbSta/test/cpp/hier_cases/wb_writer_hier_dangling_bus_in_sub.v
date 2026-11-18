// TOP: top
// TECH: nangate45
// TARGETS: hier, path_stamped_net, bus_range, wire_dcl
// CLUE: in hier mode a module-local net with no parent-side alias is reached through the
// CLUE: flat-dbNet fallback of DbInstanceNetIterator (dbNetwork.cc:400-420), so its name
// CLUE: is the full instance path.  For a BUS that path then goes through the bus
// CLUE: bookkeeping: declaration "wire [1:0] \u/w ;" must agree with the references
// CLUE: "\u/w [0]" produced by netVerilogName's separate bus-base spelling.
module sub (i, o);
  input i;
  output o;
  wire [1:0] w;
  INV_X1 g (.A(i), .ZN(o));
  INV_X1 d0 (.A(i), .ZN(w[0]));
  INV_X1 d1 (.A(i), .ZN(w[1]));
endmodule

module top (a, y);
  input a;
  output y;
  sub u (.i(a), .o(y));
endmodule
