module top(input clk, output hi, output lo);
  wire tie_hi_net;
  wire tie_lo_net;

  sky130_fd_sc_hd__conb_1 tie_hi (
    .HI(tie_hi_net)
  );
  sky130_fd_sc_hd__conb_1 tie_lo (
    .LO(tie_lo_net)
  );

  sky130_fd_sc_hd__dfxtp_1 hi_flop (
    .D(tie_hi_net),
    .Q(hi),
    .CLK(clk)
  );
  sky130_fd_sc_hd__dfxtp_1 lo_flop (
    .D(tie_lo_net),
    .Q(lo),
    .CLK(clk)
  );
endmodule
