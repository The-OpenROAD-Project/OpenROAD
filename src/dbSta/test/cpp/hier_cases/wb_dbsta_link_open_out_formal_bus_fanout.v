// TOP: top
// TECH: nangate45
// TARGETS: unconnected_formal, hasTerminals_skip, internal_fanout, bus
// CLUE: bus flavour of the open-formal/hasTerminals skip: both bits of the
// open output bus t[1:0] also feed an internal AND, so if makeDbNets drops the
// port nets the whole internal cone loses its drivers. hier additionally has
// to invent _NC filler wires for the open vector formal.
module top (a, y);
   input [1:0] a;
   output y;
   sub u (.i(a), .o(y), .t());
endmodule

module sub (i, o, t);
   input [1:0] i;
   output o;
   output [1:0] t;
   INV_X1 g0 (.A(i[0]), .ZN(t[0]));
   INV_X1 g1 (.A(i[1]), .ZN(t[1]));
   AND2_X1 g2 (.A1(t[0]), .A2(t[1]), .ZN(o));
endmodule
