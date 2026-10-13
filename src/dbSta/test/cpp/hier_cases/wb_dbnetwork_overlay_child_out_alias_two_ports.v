// TOP: top
// TECH: nangate45
// TARGETS: hier, output_port_alias, two_output_ports, one_internal_net
// CLUE: two output ports aliased to one internal net creates TWO zombie port nets
// from one merge chain (mergeAssignNet runs twice: o1->w, o2->w). Both modbterms
// should end up on empty modnets, so both child outputs lose their driver while
// the flat side stays correct. Receivers are internal top wires to stay clear of
// the known top-output duplicate-assign shape.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   wire t0;
   wire t1;
   m u1 (.i(a), .o1(t0), .o2(t1));
   BUF_X1 gt0 (.A(t0), .Z(y0));
   INV_X1 gt1 (.A(t1), .ZN(y1));
endmodule

module m (i, o1, o2);
   input i;
   output o1;
   output o2;
   wire w;
   INV_X1 g1 (.A(i), .ZN(w));
   assign o1 = w;
   assign o2 = w;
endmodule
