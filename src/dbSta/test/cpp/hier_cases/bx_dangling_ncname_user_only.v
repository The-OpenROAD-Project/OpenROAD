// TOP: top
// TECH: nangate45
// TARGETS: nc_filler_name_collision_control
// CLUE: control for the _NC1 collision: user wire named _NC1 in live logic but
//       NO unconnected bus port, so the writer should not invent fillers.
//       Isolates the collision to filler generation.
module top (x, y);
  input x;
  output y;
  wire _NC1;
  BUF_X1 u1 (.A(x), .Z(_NC1));
  INV_X1 u2 (.A(_NC1), .ZN(y));
endmodule
