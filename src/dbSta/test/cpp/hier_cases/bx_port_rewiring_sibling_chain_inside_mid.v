// TOP: top
// TECH: nangate45
// TARGETS: sibling_chain, depth_2, no_parent_logic
// CLUE: The sibling chain is one level down: top has a single child which
// CLUE: itself only wires two grandchildren together.

module top (a, y);
 input a;
 output y;
 mid u (.i(a), .o(y));
endmodule

module mid (i, o);
 input i;
 output o;
 wire m;
 sa u1 (.i(i), .o(m));
 sb u2 (.i(m), .o(o));
endmodule

module sa (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule

module sb (i, o);
 input i;
 output o;
 INV_X1 g (.A(i), .ZN(o));
endmodule
