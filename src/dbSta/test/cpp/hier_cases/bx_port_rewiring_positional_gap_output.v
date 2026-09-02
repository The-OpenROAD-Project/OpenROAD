// TOP: top
// TECH: nangate45
// TARGETS: positional_conn, empty_positional, dangling_out
// CLUE: An empty positional slot: sub u (a, , y). The skipped port is a child
// CLUE: OUTPUT that is driven inside the child but unconnected at the parent.

module top (a, y);
 input a;
 output y;
 sub u (a, , y);
endmodule

module sub (i, dz, o);
 input i;
 output dz, o;
 BUF_X1 b (.A(i), .Z(o));
 INV_X1 n (.A(i), .ZN(dz));
endmodule
