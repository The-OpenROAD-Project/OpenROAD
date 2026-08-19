// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, decl_after_items
// CLUE: Port declarations after the instance, with no implicitly created net
// CLUE: involved: exercises module_item order independence only.

module top (a, y);
 input a;
 output y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 INV_X1 g (.A(i), .ZN(o));
 input i;
 output o;
endmodule
