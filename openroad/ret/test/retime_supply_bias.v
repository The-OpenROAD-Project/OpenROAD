// retime_basic in a library with a well pin, with the supply pins wired in
// the netlist: the wells are on a bias net of their own.
module top (a, b, clk, y, VDD, VBIAS, VSS);
  input a;
  input b;
  input clk;
  output y;
  inout VDD;
  inout VBIAS;
  inout VSS;
  wire n0, q1, n1, n2, n3, n4, q2;

  BAND2_X1 g0 (.A1(a), .A2(b), .ZN(n0), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BDFF_X1 r1 (.D(n0), .CK(clk), .Q(q1), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BINV_X1 g1 (.A(q1), .ZN(n1), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BINV_X1 g2 (.A(n1), .ZN(n2), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BINV_X1 g3 (.A(n2), .ZN(n3), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BINV_X1 g4 (.A(n3), .ZN(n4), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BDFF_X1 r2 (.D(n4), .CK(clk), .Q(q2), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
  BINV_X1 g5 (.A(q2), .ZN(y), .VPWR(VDD), .VPB(VBIAS), .VGND(VSS));
endmodule
