// TOP: top
// TECH: nangate45
// TARGETS: assign_emission, bus_bit_alias, hier, netVerilogName_for_port
// CLUE: writeAssigns spells the LHS *port* with netVerilogName (VerilogWriter.cc:460),
// CLUE: the only place a port is spelled that way.  A whole-bus alias inside a submodule
// CLUE: merges bit by bit, so the module body must emit per-bit "assign o[0] = q[0];"
// CLUE: -- and if the surviving net name is the port's, no assign at all.
module sub (i, o);
  input [1:0] i;
  output [1:0] o;
  wire [1:0] q;
  INV_X1 g0 (.A(i[0]), .ZN(q[0]));
  BUF_X1 g1 (.A(i[1]), .Z(q[1]));
  assign o = q;
endmodule

module top (a, z);
  input [1:0] a;
  output [1:0] z;
  sub u (.i(a), .o(z));
endmodule
