// TOP: top
// TECH: nangate45
// TARGETS: sub_bus_input_unconnected, nc_filler_name_collision
// CLUE: hier writer fills unconnected bus ports with invented wires _NC1..;
//       here top ALREADY declares a wire _NC1 (dangling but also feeding a
//       dead gate). Probe whether the invented filler collides/aliases.
module sub (a, db, y);
  input a;
  input [3:0] db;
  output y;
  INV_X1 u1 (.A(a), .ZN(y));
endmodule

module top (x, y);
  input x;
  output y;
  wire _NC1;
  wire d0;
  BUF_X1 u1 (.A(x), .Z(_NC1));
  INV_X1 u2 (.A(_NC1), .ZN(d0));
  sub u0 (.a(x), .y(y));
endmodule
