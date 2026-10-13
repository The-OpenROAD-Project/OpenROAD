// TOP: sub_u1
// TECH: nangate45
// TARGETS: uniquification, clone_name_vs_top_module_name
// CLUE: makeUniqueDbModule builds clone names as <module>_<inst>. The block's
// top dbModule is created from the top cell name (dbBlock.cpp:562), so naming
// the top module sub_u1 puts the top module in the way of the clone that
// instance u1 of module sub wants. Third collision target after user modules
// and liberty cells.
module sub_u1 (a, y);
   input [1:0] a;
   output [1:0] y;
   sub u1 (.i(a[0]), .o(y[0]));
   sub u2 (.i(a[1]), .o(y[1]));
endmodule

module sub (i, o);
   input i;
   output o;
   INV_X1 g (.A(i), .ZN(o));
endmodule
