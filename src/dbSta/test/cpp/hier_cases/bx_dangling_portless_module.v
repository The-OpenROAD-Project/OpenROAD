// TOP: top
// TECH: nangate45
// TARGETS: portless_module, isolated_island
// CLUE: module isol has NO ports and contains a LOGIC1->INV dead island; instantiated
// as isol u_isol (); — does the instance / module content survive?
module isol ();
  wire c;
  wire d;
  LOGIC1_X1 u_c (.Z(c));
  INV_X1 u_i (.A(c), .ZN(d));
endmodule
module top (input in1, output out1);
  INV_X1 g1 (.A(in1), .ZN(out1));
  isol u_isol ();
endmodule
