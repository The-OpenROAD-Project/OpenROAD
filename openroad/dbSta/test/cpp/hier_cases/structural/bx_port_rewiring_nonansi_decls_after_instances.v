// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, decl_after_items
// CLUE: Port declarations placed AFTER the instances in the module body -
// CLUE: legal Verilog-2005 module_item ordering, unusual for a netlist.

module top (a, y);
 input a;
 output y;
 sub u (.i(a), .o(y));
endmodule

module sub (i, o);
 INV_X1 g (.A(i), .ZN(w));
 BUF_X1 b (.A(w), .Z(o));
 wire w;
 input i;
 output o;
endmodule
