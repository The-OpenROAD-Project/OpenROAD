// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, implicit_net, depth_1
// CLUE: A child output feeds a sibling child input through an UNDECLARED net
// CLUE: (implicit scalar wire created by the port connection).

module top (a, y);
 input a;
 output y;
 sub1 u1 (.i(a), .o(mid));
 sub2 u2 (.i(mid), .o(y));
endmodule

module sub1 (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule

module sub2 (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule
