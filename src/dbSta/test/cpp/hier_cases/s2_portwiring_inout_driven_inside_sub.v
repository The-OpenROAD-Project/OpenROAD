// TARGETS: inout, driven_inside_submodule, depth_1
// CLUE: The submodule's only non-input port is an INOUT, and it is DRIVEN from
// CLUE: inside the submodule; the parent reads it through an ordinary wire.
// CLUE: An inout that is a pure output at this instance is the direction case
// CLUE: the boundary code has no reason to get right, and it is observable at
// CLUE: y. The inout stays internal so the oracle keeps a driven cone.

module top (a, y);
 input a;
 output y;
 wire w;
 sub u (.p(a), .t(w));
 INV_X1 r (.A(w), .ZN(y));
endmodule

module sub (p, t);
 input p;
 inout t;
 BUF_X1 d (.A(p), .Z(t));
endmodule
