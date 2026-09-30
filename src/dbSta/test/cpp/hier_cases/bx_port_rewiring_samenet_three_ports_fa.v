// TOP: top
// TECH: nangate45
// TARGETS: shared_net, three_pins, FA_X1
// CLUE: One net into all three inputs of FA_X1 inside a child; both CO and S
// CLUE: reach top outputs.

module top (a, co, s);
 input a;
 output co, s;
 sub u (.x(a), .c(co), .d(s));
endmodule

module sub (x, c, d);
 input x;
 output c, d;
 FA_X1 g (.A(x), .B(x), .CI(x), .CO(c), .S(d));
endmodule
