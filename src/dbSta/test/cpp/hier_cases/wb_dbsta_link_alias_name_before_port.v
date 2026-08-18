// TOP: top
// TECH: nangate45
// TARGETS: assign_alias_name_order, findNet_by_port_name, hier
// CLUE: makeModNetsForSubmodule pushes into the child with
// network_->findNet(inst, <port name>) (dbReadVerilog.cc:888). When an assign
// merges the port net with an internal wire, sta keeps only ONE name for the
// merged net; if the wire name sorts before the port name ("aa" < "i") the
// lookup by port name fails and the child port gets no lower modnet at all.
module top (a, y);
   input a;
   output y;
   sub u (.i(a), .o(y));
endmodule

module sub (i, o);
   input i;
   output o;
   wire aa;
   assign aa = i;
   INV_X1 g (.A(aa), .ZN(o));
endmodule
