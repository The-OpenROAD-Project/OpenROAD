// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, fanout_3, no_parent_logic
// CLUE: One child output net simultaneously drives two sibling child input
// CLUE: ports and a top output, with no buffering at the parent.

module top (a, y0, y1, y2);
 input a;
 output y0, y1, y2;
 wire m;
 src u0 (.i(a), .o(m));
 snk u1 (.i(m), .o(y0));
 snk2 u2 (.i(m), .j(m), .o(y1));
 assign y2 = m;
endmodule

module src (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule

module snk (i, o);
 input i;
 output o;
 BUF_X1 g (.A(i), .Z(o));
endmodule

module snk2 (i, j, o);
 input i, j;
 output o;
 XNOR2_X1 g (.A(i), .B(j), .ZN(o));
endmodule
