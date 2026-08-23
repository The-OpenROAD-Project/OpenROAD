// TOP: top
// TECH: nangate45
// TARGETS: nonansi_header_order, named_conn
// CLUE: Header (b, a) with declarations input a; output b;. Named binding, so
// CLUE: only the emitted port ORDER can go wrong.

module top (a, y);
 input a;
 output y;
 sub u (.a(a), .b(y));
endmodule

module sub (b, a);
 input a;
 output b;
 INV_X1 g (.A(a), .ZN(b));
endmodule
