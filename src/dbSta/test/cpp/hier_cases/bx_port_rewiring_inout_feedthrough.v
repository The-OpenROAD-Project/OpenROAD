// TOP: top
// TECH: nangate45
// TARGETS: inout, feedthrough, probe
// CLUE: PROBE: a top-level inout is fed through to a child inout port and
// CLUE: read there. May be rejected or mismodelled by reader/oracle.

module top (a, io, y);
 input a;
 inout io;
 output y;
 sub u (.p(a), .t(io), .z(y));
endmodule

module sub (p, t, z);
 input p;
 inout t;
 output z;
 XOR2_X1 g (.A(p), .B(t), .Z(z));
endmodule
