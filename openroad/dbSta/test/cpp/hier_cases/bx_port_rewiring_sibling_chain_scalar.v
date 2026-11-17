// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, depth_1, no_parent_logic
// CLUE: Child output feeds sibling child input with no logic at the parent -
// CLUE: the parent contributes only a wire between two hierarchy boundaries.

module top (a, y);
 input a;
 output y;
 wire m;
 sa u1 (.i(a), .o(m));
 sb u2 (.i(m), .o(y));
endmodule

module sa (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule

module sb (i, o);
 input i;
 output o;
 BUF_X1 g (.A(i), .Z(o));
endmodule
