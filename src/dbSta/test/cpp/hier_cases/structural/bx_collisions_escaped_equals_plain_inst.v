// TOP: top
// TECH: nangate45
// TARGETS: escaped_identity, duplicate_inst, reader_probe
// CLUE: \abc and abc denote the SAME identifier per LRM, so two instances
// with these names are duplicates -- illegal input; probes reader detection.
module top (input in1, input in2, output o1, output o2);
  INV_X1 abc (.A(in1), .ZN(o1));
  INV_X1 \abc  (.A(in2), .ZN(o2));
endmodule
