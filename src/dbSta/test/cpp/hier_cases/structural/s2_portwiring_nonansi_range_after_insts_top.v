// TARGETS: nonansi_header_order, decl_after_instances, top_level, bus
// CLUE: Same shape as the submodule variant but the late bus declaration is on
// CLUE: the TOP module, whose port ranges also feed the bus_msb_first cookie
// CLUE: recordBusPortsOrder writes for the writer.

module top (a, y);
 sub u (.i(a), .o(y));
 input [3:0] a;
 output [3:0] y;
endmodule

module sub (i, o);
 input [3:0] i;
 output [3:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[1]));
 INV_X1 b1 (.A(i[1]), .ZN(o[0]));
 BUF_X1 b2 (.A(i[2]), .Z(o[3]));
 INV_X1 b3 (.A(i[3]), .ZN(o[2]));
endmodule
