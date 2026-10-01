// TOP: top
// TECH: nangate45
// TARGETS: control, depth_1, module_local_net
// CLUE: Baseline control for the whole family: one module-local net driven by a
// plainly-named leaf, so dbNetwork::name(Net) (dbNetwork.cc:1850-1868) takes the
// happy path (driver ITerm found, header_to_remove == "u1" is a prefix of the
// flat name "u1/w") and the net must come back as `w` inside module m.
module top (a, y);
   input a;
   output y;
   m u1 (.i(a), .o(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   BUF_X1 g2 (.A(w), .Z(o));
endmodule
