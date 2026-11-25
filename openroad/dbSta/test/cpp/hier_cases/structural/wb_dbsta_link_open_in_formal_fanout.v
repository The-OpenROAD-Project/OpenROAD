// TOP: top
// TECH: nangate45
// TARGETS: unconnected_formal, hasTerminals_skip, internal_fanout, input_side
// CLUE: input-side mirror of the open-formal skip. Child net i has a terminal
// (port i) but no parent net, and it fans out to two internal loads; the
// surviving structure tells us whether makeDbNets dropped the net and both
// gate connections with it. Top output y1 stays driven so only the y0 cone is
// affected.
module top (a, y0, y1);
   input a;
   output y0;
   output y1;
   sub u (.i(), .j(a), .o0(y0), .o1(y1));
endmodule

module sub (i, j, o0, o1);
   input i;
   input j;
   output o0;
   output o1;
   XOR2_X1 g0 (.A(i), .B(j), .Z(o0));
   AND2_X1 g1 (.A1(i), .A2(j), .ZN(o1));
endmodule
