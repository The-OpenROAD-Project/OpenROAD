// TOP: top
// TECH: nangate45
// TARGETS: hier, name_erase_overshoot, escaped_slash, port_capture, short
// CLUE: dbNetwork::name(Net) erases header_to_remove.length()+1 chars at the
// FIRST find() hit (dbNetwork.cc:1862-1865). With driver `\g/1 ` the header is
// the malformed "u1/g\" (see ..._netname_drvr_escslash); for a net named `\g/x `
// the flat name "u1/g\/x" DOES start with that header, so 6 chars are erased and
// the net is renamed to plain "x" -- the name of an unrelated input PORT of the
// same module. Predict y0 gets driven from port x instead of ~a.
module top (a, b, y0, y1);
   input a;
   input b;
   output y0;
   output y1;
   m u1 (.a(a), .x(b), .y0(y0), .y1(y1));
endmodule

module m (a, x, y0, y1);
   input a;
   input x;
   output y0;
   output y1;
   wire \g/x ;
   INV_X1 \g/1  (.A(a), .ZN(\g/x ));
   BUF_X1 g2 (.A(\g/x ), .Z(y0));
   BUF_X1 g3 (.A(x), .Z(y1));
endmodule
