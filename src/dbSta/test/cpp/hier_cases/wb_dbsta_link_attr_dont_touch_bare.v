// TOP: top
// TECH: nangate45
// TARGETS: attribute, dont_touch, control
// CLUE: control for the dont_touch stoi path: a bare attribute gets the value
// "1" from the grammar (attr_spec: ID) and an explicit integer parses too, so
// both should link and round-trip cleanly.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   (* dont_touch *)
   INV_X1 g0 (.A(a), .ZN(y0));
   (* dont_touch = 1 *)
   BUF_X1 g1 (.A(b), .Z(y1));
endmodule
