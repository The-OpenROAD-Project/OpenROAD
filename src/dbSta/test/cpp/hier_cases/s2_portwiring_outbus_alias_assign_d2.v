// TARGETS: output_port_alias, bus, whole_bus_assign, depth_2
// CLUE: The deepest module drives an internal bus t and aliases the whole of it
// CLUE: onto its output port with one vector assign, so the net that actually
// CLUE: carries each bit is named t[k], not o[k]. The hierarchical push-down
// CLUE: looks the internal net up by PORT name, which is the name that did not
// CLUE: survive the merge.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input [3:0] i;
 output [3:0] o;
 sub u (.i(i), .o(o));
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 wire [3:0] t;
 INV_X1 g0 (.A(i[0]), .ZN(t[1]));
 BUF_X1 g1 (.A(i[1]), .Z(t[0]));
 INV_X1 g2 (.A(i[2]), .ZN(t[3]));
 BUF_X1 g3 (.A(i[3]), .Z(t[2]));
 assign o = t;
endmodule
