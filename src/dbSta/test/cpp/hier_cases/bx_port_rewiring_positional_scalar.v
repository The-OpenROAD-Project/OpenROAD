// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, scalar_ports, depth_1
// CLUE: Purely positional port connections on a scalar-port child.

module top (a, b, y);
 input a, b;
 output y;
 sub u (a, b, y);
endmodule

module sub (p, q, z);
 input p, q;
 output z;
 NAND2_X1 g (.A1(p), .A2(q), .ZN(z));
endmodule
