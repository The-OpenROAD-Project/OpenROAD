// TOP: top
// TECH: nangate45
// TARGETS: portname_collision, opposite_directions
// CLUE: The port name x is an OUTPUT of one master and an INPUT of another;
// CLUE: the same parent net binds both. Flattening merges the two namespaces.

module top (a, y);
 input a;
 output y;
 wire x;
 ma u1 (.x(x), .p(a));
 mb u2 (.x(x), .z(y));
endmodule

module ma (x, p);
 output x;
 input p;
 INV_X1 g (.A(p), .ZN(x));
endmodule

module mb (x, z);
 input x;
 output z;
 BUF_X1 g (.A(x), .Z(z));
endmodule
