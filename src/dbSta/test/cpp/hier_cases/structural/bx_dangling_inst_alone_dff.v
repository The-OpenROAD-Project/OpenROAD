// TOP: top
// TECH: nangate45
// TARGETS: instance_all_pins_unconnected, sequential_cell
// CLUE: DFF_X1 u_alone (); fully unconnected sequential cell. Register cells
//       often take a different code path in writers than combinational ones.
module top (x, y);
  input x;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
  DFF_X1 u_alone ();
endmodule
