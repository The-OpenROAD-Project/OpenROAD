// TARGETS: bus_split_two_drivers, concat_output_port, two_instances, depth_1
// CLUE: One top output bus is driven bit by bit from two instances of the same
// CLUE: master, interleaved rather than split into contiguous halves: u1 owns
// CLUE: y[3] and y[1], u2 owns y[2] and y[0], expressed as concats on the
// CLUE: OUTPUT port connection. The input side is interleaved the same way.

module top (a, y);
 input [3:0] a;
 output [3:0] y;
 h u1 (.i({a[3],a[1]}), .o({y[3],y[1]}));
 h u2 (.i({a[2],a[0]}), .o({y[2],y[0]}));
endmodule

module h (i, o);
 input [1:0] i;
 output [1:0] o;
 BUF_X1 b0 (.A(i[0]), .Z(o[0]));
 INV_X1 b1 (.A(i[1]), .ZN(o[1]));
endmodule
