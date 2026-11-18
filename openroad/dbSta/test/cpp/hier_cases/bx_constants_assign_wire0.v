// TOP: top
// TECH: nangate45
// TARGETS: assign_const_wire
// CLUE: assign w = 1'b0 then w consumed by a gate — writers may re-emit the
// assign, tie the pin directly, or insert a LOGIC0 cell.
module top (input a, output y);
  wire w;
  assign w = 1'b0;
  OR2_X1 g1 (.A1(a), .A2(w), .ZN(y));
endmodule
