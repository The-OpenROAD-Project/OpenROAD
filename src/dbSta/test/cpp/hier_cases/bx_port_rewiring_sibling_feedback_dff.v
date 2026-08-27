// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, feedback, sequential
// CLUE: Child output goes to a sibling child input and, through a flop, back
// CLUE: into the first child: a hierarchy-crossing feedback ring.

module top (a, clk, y);
 input a, clk;
 output y;
 wire m, n, q;
 sa u1 (.i0(a), .i1(q), .o(m));
 sb u2 (.i(m), .o(n));
 DFF_X1 r (.D(n), .CK(clk), .Q(q));
 BUF_X1 g (.A(n), .Z(y));
endmodule

module sa (i0, i1, o);
 input i0, i1;
 output o;
 XOR2_X1 g (.A(i0), .B(i1), .Z(o));
endmodule

module sb (i, o);
 input i;
 output o;
 BUF_X1 g (.A(i), .Z(o));
endmodule
