// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, depth_2, mid_module
// CLUE: same output-port alias hazard one level down: the alias lives in the MID
// module and the driver is a child MODULE output (a modITerm, not an iterm), so
// the fallback path in dbNetwork::net(Pin) for moditerms -- which returns ONLY the
// modnet, never a dbNet (dbNetwork.cc:1438-1442) -- decides whether mid's port o
// keeps any driver at all.
module top (a, y);
   input a;
   output y;
   m1 u1 (.i(a), .o(y));
endmodule

module m1 (i, o);
   input i;
   output o;
   wire w;
   m2 u2 (.i(i), .o(w));
   assign o = w;
endmodule

module m2 (i, o);
   input i;
   output o;
   INV_X1 g1 (.A(i), .ZN(o));
endmodule
