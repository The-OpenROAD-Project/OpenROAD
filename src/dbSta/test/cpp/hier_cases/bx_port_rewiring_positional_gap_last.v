// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, empty_positional, last_slot
// CLUE: Empty LAST positional slot: sub u (a, y, ). Skipped port is an unused
// CLUE: child output.

module top (a, y);
 input a;
 output y;
 sub u (a, y, );
endmodule

module sub (i, o, dz);
 input i;
 output o, dz;
 BUF_X1 b (.A(i), .Z(o));
 INV_X1 n (.A(i), .ZN(dz));
endmodule
