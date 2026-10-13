// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, bus_port, per_bit_merge
// CLUE: bus form of the output-port alias. mergeAssignNet bit-blasts the assign,
// so o[1] and o[0] each become zombie nets while w[1], w[0] survive; the child bus
// modbterms are then pushed with findNet(inst,"o[1]")/("o[0]") and should both get
// empty modnets. Exercises the bus-member path of dbNetwork::findMember /
// memberIterator on top of the alias hazard.
module top (a, y);
   input [1:0] a;
   output [1:0] y;
   wire [1:0] t;
   m u1 (.i(a), .o(t));
   BUF_X1 gt0 (.A(t[0]), .Z(y[0]));
   BUF_X1 gt1 (.A(t[1]), .Z(y[1]));
endmodule

module m (i, o);
   input [1:0] i;
   output [1:0] o;
   wire [1:0] w;
   INV_X1 g0 (.A(i[0]), .ZN(w[0]));
   INV_X1 g1 (.A(i[1]), .ZN(w[1]));
   assign o = w;
endmodule
