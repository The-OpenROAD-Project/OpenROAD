// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_bit_dangling, concat_dead_wire
// CLUE: one bit of sub bus input db is fed by declared-but-undriven wire dead
//       inside a concat: .db({x3,x2,x1,dead}). db is unused inside sub, so
//       this is purely structural.
module sub (a, db, y);
  input a;
  input [3:0] db;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, x1, x2, x3, y);
  input x;
  input x1;
  input x2;
  input x3;
  output y;
  wire dead;
  sub u0 (.a(x), .db({x3, x2, x1, dead}), .y(y));
endmodule
