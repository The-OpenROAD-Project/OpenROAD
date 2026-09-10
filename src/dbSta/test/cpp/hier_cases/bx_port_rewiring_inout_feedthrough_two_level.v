// TOP: top
// TECH: nangate45
// TARGETS: inout, feedthrough, depth_2, probe
// CLUE: PROBE: inout carried through two hierarchy levels before being read.

module top (a, io, y);
 input a;
 inout io;
 output y;
 mid u (.p(a), .t(io), .z(y));
endmodule

module mid (p, t, z);
 input p;
 inout t;
 output z;
 sub u (.p(p), .t(t), .z(z));
endmodule

module sub (p, t, z);
 input p;
 inout t;
 output z;
 XNOR2_X1 g (.A(p), .B(t), .ZN(z));
endmodule
