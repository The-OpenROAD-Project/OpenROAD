// TARGETS: bus_split_rejoin, part_select_port_conn, concat_port_conn, depth_4
// CLUE: A 4-bit bus is split into two 2-bit ports with the halves crossed,
// CLUE: rejoined by a concat one level down, split again by parity (even bits
// CLUE: to one port, odd bits to another) and finally consumed at depth 4.
// CLUE: No level has a gate except the leaf, so the whole case is wiring.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 l1 u (.i(a), .o(y));
endmodule

module l1 (i, o);
 input [3:0] i;
 output [3:0] o;
 l2 u (.hi(i[3:2]), .lo(i[1:0]), .o(o));
endmodule

module l2 (hi, lo, o);
 input [1:0] hi;
 input [1:0] lo;
 output [3:0] o;
 l3 u (.i({lo,hi}), .o(o));
endmodule

module l3 (i, o);
 input [3:0] i;
 output [3:0] o;
 l4 u (.ev({i[2],i[0]}), .od({i[3],i[1]}), .o(o));
endmodule

module l4 (ev, od, o);
 input [1:0] ev;
 input [1:0] od;
 output [3:0] o;
 BUF_X1 g0 (.A(ev[0]), .Z(o[0]));
 INV_X1 g1 (.A(od[0]), .ZN(o[1]));
 BUF_X1 g2 (.A(ev[1]), .Z(o[2]));
 INV_X1 g3 (.A(od[1]), .ZN(o[3]));
endmodule
