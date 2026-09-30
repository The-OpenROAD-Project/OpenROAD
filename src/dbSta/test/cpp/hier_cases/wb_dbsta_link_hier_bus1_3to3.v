// TOP: top
// TECH: nangate45
// TARGETS: single_bit_bus_port, nonzero_single_index, hier
// CLUE: one-bit bus at a non-zero index [3:3]: from == to so recordBusPortsOrder
// records msb_first = (from > to) = FALSE even though the declaration is a
// descending range, and the only bit modbterm is named "d[3]" while the
// sentinel is "d". Probes whether the bit index survives the round trip.
module top (a, y);
   input a;
   output y;
   sub u (.d(a), .q(y));
endmodule

module sub (d, q);
   input [3:3] d;
   output q;
   INV_X1 g (.A(d[3]), .ZN(q));
endmodule
