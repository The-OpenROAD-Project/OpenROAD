// TOP: top
// TECH: nangate45
// TARGETS: 4bit_dff_bank_in_submodule, reversed_bit_order_at_instantiation
// CLUE: the same bank is instantiated with both bus actuals reversed by
// explicit concatenation, so bit i of the parent maps to bit 3-i of the child.

module bank (input [3:0] d, input ck, output [3:0] q);
  DFF_X1 f0 (.D(d[0]), .CK(ck), .Q(q[0]));
  DFF_X1 f1 (.D(d[1]), .CK(ck), .Q(q[1]));
  DFF_X1 f2 (.D(d[2]), .CK(ck), .Q(q[2]));
  DFF_X1 f3 (.D(d[3]), .CK(ck), .Q(q[3]));
endmodule

module top (input [3:0] d, input ck, output [3:0] q);
  bank u (.d({d[0], d[1], d[2], d[3]}), .ck(ck),
          .q({q[0], q[1], q[2], q[3]}));
endmodule
