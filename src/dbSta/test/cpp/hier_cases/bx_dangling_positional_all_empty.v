// TOP: top
// TECH: nangate45
// TARGETS: sub_all_ports_unconnected, positional_empty_slots
// CLUE: instance with an all-empty ordered connection list: sub u_alone ( , , );
// the extreme form of the positional empty slot.
module sub (a, b, y);
  input a;
  input b;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule
module top (input x, output y);
  INV_X1 g1 (.A(x), .ZN(y));
  sub u_alone ( , , );
endmodule
