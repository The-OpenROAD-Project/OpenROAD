// retime_basic in a library with a well pin, inside a module, with the
// supply pins wired through the module's ports: the cells read rail, well
// and gnd, which are vdd_main, vbias and vss one level up.  The wells are on
// a bias net of their own.
module core (a, b, clk, y, rail, well, gnd);
  input a;
  input b;
  input clk;
  output y;
  input rail;
  input well;
  input gnd;
  wire n0, q1, n1, n2, n3, n4, q2;

  BAND2_X1 g0 (.A1(a), .A2(b), .ZN(n0), .VPWR(rail), .VPB(well), .VGND(gnd));
  BDFF_X1 r1 (.D(n0), .CK(clk), .Q(q1), .VPWR(rail), .VPB(well), .VGND(gnd));
  BINV_X1 g1 (.A(q1), .ZN(n1), .VPWR(rail), .VPB(well), .VGND(gnd));
  BINV_X1 g2 (.A(n1), .ZN(n2), .VPWR(rail), .VPB(well), .VGND(gnd));
  BINV_X1 g3 (.A(n2), .ZN(n3), .VPWR(rail), .VPB(well), .VGND(gnd));
  BINV_X1 g4 (.A(n3), .ZN(n4), .VPWR(rail), .VPB(well), .VGND(gnd));
  BDFF_X1 r2 (.D(n4), .CK(clk), .Q(q2), .VPWR(rail), .VPB(well), .VGND(gnd));
  BINV_X1 g5 (.A(q2), .ZN(y), .VPWR(rail), .VPB(well), .VGND(gnd));
endmodule

module top (a, b, clk, y, vdd_main, vbias, vss);
  input a;
  input b;
  input clk;
  output y;
  input vdd_main;
  input vbias;
  input vss;

  core u_core (.a(a), .b(b), .clk(clk), .y(y),
               .rail(vdd_main), .well(vbias), .gnd(vss));
endmodule
