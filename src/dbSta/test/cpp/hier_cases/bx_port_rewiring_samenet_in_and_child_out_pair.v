// TOP: top
// TECH: nangate45
// TARGETS: shared_net, cross_instance, depth_2
// CLUE: The same parent net is an input port of one child and an input port
// CLUE: of a grandchild reached through a second child - one net crossing two
// CLUE: different hierarchy boundaries at different depths.

module top (a, y0, y1);
 input a;
 output y0, y1;
 lo u1 (.p(a), .z(y0));
 hi u2 (.p(a), .z(y1));
endmodule

module lo (p, z);
 input p;
 output z;
 INV_X1 g (.A(p), .ZN(z));
endmodule

module hi (p, z);
 input p;
 output z;
 lo u (.p(p), .z(z));
endmodule
