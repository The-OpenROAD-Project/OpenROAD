module OPENROAD_CLKGATE (CK, E, GCK);
  input CK;
  input E;
  output GCK;

`ifdef OPENROAD_CLKGATE

ICGx1_ASAP7_75t_R latch ( .CLK(CK), .ENA(E), .SE(1'b0), .GCLK(GCK) );

`else

assign GCK = CK;

`endif

endmodule
