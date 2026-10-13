// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, nonansi_header_order, decl_order_mismatch
// CLUE: Child header order is (b, a) but the declarations are 'input a;
// CLUE: output b;'. Positional binding MUST follow header order, not
// CLUE: declaration order; getting it wrong swaps a driver and a load.

module top (a, y);
 input a;
 output y;
 sub u (y, a);
endmodule

module sub (b, a);
 input a;
 output b;
 INV_X1 g (.A(a), .ZN(b));
endmodule
