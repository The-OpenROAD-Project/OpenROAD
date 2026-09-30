// TARGETS: escaped_net, keyword_wire, bus, depth_1
// CLUE: a keyword-named BUS reaches the unescaping defect by a different route
// than the covered scalar case (bx_naming_escaped_net_kw_wire.v). writeWireDcls
// collapses bus bits into one declaration and prints the BASE name through
// netVerilogName (VerilogWriter.cc:296-300), which for a bus takes the
// staToVerilog branch on the base only (VerilogNamespace.cc:59-64). "wire" is
// all alnum, so staToVerilog (VerilogNamespace.cc:83-115) never sets the escape
// flag and the declaration should come out as `wire [1:0] wire;`.
module top (a, y0, y1);
  input a;
  output y0;
  output y1;
  wire [1:0] \wire ;
  INV_X1 g0 (.A(a), .ZN(\wire [0]));
  BUF_X1 g1 (.A(a), .Z(\wire [1]));
  BUF_X1 h0 (.A(\wire [0]), .Z(y0));
  INV_X1 h1 (.A(\wire [1]), .ZN(y1));
endmodule
