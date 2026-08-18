// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, modnet_missing, net_pin_modnet_preference
// CLUE: `assign o = w;` is merged by VerilogReader::mergeAssignNet as
// mergeInto(lhs,rhs) -> the RHS net w survives and the port net o becomes a
// zombie (no pins/terms). makeModNetsForSubmodule then pushes into the child with
// findNet(inst,"o") (dbReadVerilog.cc:888), gets the zombie, and builds an EMPTY
// modnet "o". The real driver iterm keeps only its flat dbNet, so
// dbNetwork::net(Pin) (dbNetwork.cc:1425) falls back to the dbNet and the writer
// should emit .ZN(<top-level flat name>) inside the module, leaving port o
// undriven. Receiver is an internal top wire, so this is NOT the known
// "duplicate assign on a top output" shape.
module top (a, y);
   input a;
   output y;
   wire t;
   m u1 (.i(a), .o(t));
   BUF_X1 gt (.A(t), .Z(y));
endmodule

module m (i, o);
   input i;
   output o;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   assign o = w;
endmodule
