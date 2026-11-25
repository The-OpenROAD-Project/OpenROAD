// TARGETS: nonansi_header_order, decl_after_instances, bus, depth_1
// CLUE: The submodule's bus port ranges are declared AFTER the instance
// CLUE: statements that reference the bus bits. The reader has to finish the
// CLUE: module before it knows i and o are 4 bits wide; if a bit reference is
// CLUE: resolved eagerly the wrong net is bound. The child also permutes, so
// CLUE: every output bit is a distinct function of a distinct input bit.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 BUF_X1 b0 (.A(i[0]), .Z(o[1]));
 INV_X1 b1 (.A(i[1]), .ZN(o[0]));
 BUF_X1 b2 (.A(i[2]), .Z(o[3]));
 INV_X1 b3 (.A(i[3]), .ZN(o[2]));
 input [3:0] i;
 output [3:0] o;
endmodule
