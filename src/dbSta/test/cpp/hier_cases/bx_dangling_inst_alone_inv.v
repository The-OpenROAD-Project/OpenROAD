// TOP: top
// TECH: nangate45
// TARGETS: instance_all_pins_unconnected
// CLUE: INV_X1 u_alone (); has every pin unconnected. Fully floating leaf cell:
//       classic candidate for silent deletion by either writer.
module top (x, y);
  input x;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
  INV_X1 u_alone ();
endmodule
