// TOP: top
// TECH: nangate45
// TARGETS: shared_net, same_inst_two_ports
// CLUE: One top net drives BOTH input ports of a single child instance.

module top (a, y);
 input a;
 output y;
 sub u (.p(a), .q(a), .z(y));
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 NAND2_X1 g (.A1(p), .A2(q), .ZN(z));
endmodule
