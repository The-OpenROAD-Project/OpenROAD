// TARGETS: two_instances, uniquify, perm_swap, depth_3
// CLUE: The same 2-bit permuting master p appears at depth 1, depth 2 and
// CLUE: depth 3 on three separate branches, each reached through a different
// CLUE: chain of concat bindings. Uniquification makes three clones and each
// CLUE: clone has to keep its own binding.

module top (a, y);
 input [5:0] a;
 output [5:0] y;
 p u0 (.i(a[1:0]), .o(y[1:0]));
 m1 u1 (.i(a[3:2]), .o(y[3:2]));
 m2 u2 (.i(a[5:4]), .o(y[5:4]));
endmodule

module m1 (i, o);
 input [1:0] i;
 output [1:0] o;
 p u (.i({i[0],i[1]}), .o(o));
endmodule

module m2 (i, o);
 input [1:0] i;
 output [1:0] o;
 m3 u (.i(i), .o({o[0],o[1]}));
endmodule

module m3 (i, o);
 input [1:0] i;
 output [1:0] o;
 p u (.i(i), .o({o[0],o[1]}));
endmodule

module p (i, o);
 input [1:0] i;
 output [1:0] o;
 INV_X1 g0 (.A(i[1]), .ZN(o[0]));
 BUF_X1 g1 (.A(i[0]), .Z(o[1]));
endmodule
