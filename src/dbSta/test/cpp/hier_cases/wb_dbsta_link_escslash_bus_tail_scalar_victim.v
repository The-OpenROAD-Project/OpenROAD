// TOP: top
// TECH: nangate45
// TARGETS: escaped_slash_bus_port, scalar_port_becomes_bus, dbBusPort_hijack
// CLUE: worst form of the tail collision: the escaped port is a BUS whose
// sentinel name "x\/y" tail-matches an existing SCALAR port y, so
// dbModBTerm::create returns the scalar modbterm and dbBusPort::create +
// setBusPort turn that scalar port into a bus aggregate. makeModITerms then
// skips it (isBusPort() is now true), so the scalar port loses its modITerm and
// the parent connection to it has nowhere to land.
module top (a, b, o);
   input a;
   input [1:0] b;
   output [1:0] o;
   sub u (.y(a), .\x/y (b), .o(o));
endmodule

module sub (y, \x/y , o);
   input y;
   input [1:0] \x/y ;
   output [1:0] o;
   AND2_X1 g0 (.A1(y), .A2(\x/y [0]), .ZN(o[0]));
   OR2_X1 g1 (.A1(y), .A2(\x/y [1]), .ZN(o[1]));
endmodule
