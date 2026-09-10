// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, three_children, no_parent_logic
// CLUE: Three children chained output-to-input; the parent has no cells at all.

module top (a, y);
 input a;
 output y;
 wire m0, m1;
 sa u1 (.i(a), .o(m0));
 sb u2 (.i(m0), .o(m1));
 sa u3 (.i(m1), .o(y));
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
