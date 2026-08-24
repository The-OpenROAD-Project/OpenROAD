// TOP: top
// TECH: nangate45
// TARGETS: submodule_bus_bit_io_direction
// CLUE: mined from fd258c2516 "Fixed wrong IO type of dbModBTerm for a bus
// port", which landed with NO netlist reproducer. In makeModBTerms the bus-bit
// loop set the IO type on the bus AGGREGATE sentinel (bmodterm) instead of on
// each bit (modbterm), so individual bus-port bits kept a default direction.
// The trigger is a submodule carrying both an input and an output bus, of
// different widths, so a bit whose direction is wrong cannot be masked by a
// same-shaped port on the other side.
module top (a, y);
   input [3:0] a;
   output [1:0] y;
   bus_io u_bus (.din(a), .dout(y));
endmodule

module bus_io (din, dout);
   input [3:0] din;
   output [1:0] dout;
   AND2_X1 g0 (.A1(din[0]), .A2(din[1]), .ZN(dout[0]));
   OR2_X1  g1 (.A1(din[2]), .A2(din[3]), .ZN(dout[1]));
endmodule
