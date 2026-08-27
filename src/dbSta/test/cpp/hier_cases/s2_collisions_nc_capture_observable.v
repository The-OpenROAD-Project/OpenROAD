// TARGETS: nc_filler, sub_bus_output_unconnected, nc_filler_vs_top_port, hier
// CLUE: every existing _NC victim case leaves an INPUT bus open, so the filler
// only READS the stolen net.  Here the open formal is an OUTPUT bus that is
// driven inside sub, so writeInstBusPinBit (VerilogWriter.cc:430) hands a
// DRIVER to the user's top input port _NC1 -- and _NC1 feeds y2, so the capture
// is observable at a boundary output instead of being invisible.  The input
// netlist has no floating net, so the oracle can judge every cone.
module ncsub (input a, output [1:0] db, output y);
  INV_X1 g0 (.A(a), .ZN(y));
  BUF_X1 g1 (.A(a), .Z(db[0]));
  INV_X1 g2 (.A(a), .ZN(db[1]));
endmodule

module top (input x, input _NC1, output y, output y2);
  ncsub u0 (.a(x), .db(), .y(y));
  AND2_X1 g3 (.A1(x), .A2(_NC1), .ZN(y2));
endmodule
