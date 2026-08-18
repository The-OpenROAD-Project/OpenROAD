// TOP: top
// TECH: nangate45
// TARGETS: inout, bus, perm_swap, probe
// CLUE: PROBE: a 2-bit top-level inout bus is handed to a child inout port with
// CLUE: the bits swapped in the concat, and read there.

module top (a, io, y);
 input [1:0] a;
 inout [1:0] io;
 output [1:0] y;
 sub u (.p(a), .t({io[0],io[1]}), .z(y));
endmodule

module sub (p, t, z);
 input [1:0] p;
 inout [1:0] t;
 output [1:0] z;
 XOR2_X1 g0 (.A(p[0]), .B(t[0]), .Z(z[0]));
 XOR2_X1 g1 (.A(p[1]), .B(t[1]), .Z(z[1]));
endmodule
