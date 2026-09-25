// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire, x_value
// CLUE: assign w = 1'bx then w consumed by a gate — bx_constants_x_conn showed
// pin-context 1'bx silently becomes the zero_ net; probe the assign context.
module top (input a, output y);
  wire w;
  assign w = 1'bx;
  OR2_X1 g (.A1(a), .A2(w), .ZN(y));
endmodule
