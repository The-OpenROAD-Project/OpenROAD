// TOP: top
// TECH: nangate45
// TARGETS: explicit_header_port, name_swap, depth_1
// CLUE: Evil but legal: external port 'a' binds to internal net 'b' and
// CLUE: external port 'b' binds to internal net 'a'. External a is the INPUT.
// CLUE: Any name-keyed shortcut in the reader inverts the direction.

module top (a, y);
 input a;
 output y;
 sub u (.a(a), .b(y));
endmodule

module sub (.a(b), .b(a));
 input b;
 output a;
 INV_X1 g (.A(b), .ZN(a));
endmodule
