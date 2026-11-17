// TOP: top
// TECH: nangate45
// TARGETS: instance_all_pins_unconnected, escaped_instance_name
// CLUE: fully unconnected INV with escaped instance name \u.alone (dot in
//       name). Dangling instance + escaped-name preservation in one object.
module top (x, y);
  input x;
  output y;
  INV_X1 u1 (.A(x), .ZN(y));
  INV_X1 \u.alone ();
endmodule
