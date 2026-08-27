// TARGETS: sub_output_dangling, bus, two_modules_with_nc, depth_2
// CLUE: TWO different modules each instantiate a child with an unconnected
// CLUE: 2-bit OUTPUT bus, so the writer has to invent filler wires in both.
// CLUE: The filler counter is global to the file while the declarations are
// CLUE: emitted per module, so the second module's fillers can be numbered
// CLUE: past what that module declares.

module top (a, y, z);
 input [1:0] a;
 output [1:0] y;
 output [1:0] z;
 h u1 (.i(a), .o(y), .x());
 mid u2 (.i(a), .o(z));
endmodule

module mid (i, o);
 input [1:0] i;
 output [1:0] o;
 h u (.i({i[0],i[1]}), .o(o), .x());
endmodule

module h (i, o, x);
 input [1:0] i;
 output [1:0] o;
 output [1:0] x;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
 BUF_X1 b2 (.A(i[0]), .Z(x[0]));
 INV_X1 b3 (.A(i[1]), .ZN(x[1]));
endmodule
