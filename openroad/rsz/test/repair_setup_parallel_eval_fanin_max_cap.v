// Three scan flops on output paths with heavy loads.  The *_full output ports
// load the drivers of the tested pins past their max cap: the scan pins of
// ff_scan, the ideal clock pin of ff_ideal and the propagated clock pin of
// ff_prop.  Every other flop input has a driver with headroom.
module top (clk, clk_prop, d, se, si,
            se_full, si_full, clk_full, clk_prop_full,
            q_scan, q_ideal, q_prop);
  input clk, clk_prop, d, se, si;
  output se_full, si_full, clk_full, clk_prop_full;
  output q_scan, q_ideal, q_prop;
  wire d_buf, se_buf, si_buf;

  sky130_fd_sc_hd__buf_1 drv_d (.A(d), .X(d_buf));
  sky130_fd_sc_hd__buf_1 drv_se (.A(se), .X(se_buf));
  sky130_fd_sc_hd__buf_1 drv_si (.A(si), .X(si_buf));
  sky130_fd_sc_hd__buf_1 drv_se_full (.A(se), .X(se_full));
  sky130_fd_sc_hd__buf_1 drv_si_full (.A(si), .X(si_full));
  sky130_fd_sc_hd__clkbuf_1 drv_clk_full (.A(clk), .X(clk_full));
  sky130_fd_sc_hd__clkbuf_1 drv_clk_prop_full (.A(clk_prop),
    .X(clk_prop_full));

  sky130_fd_sc_hd__sdfxbp_1 ff_scan (.CLK(clk), .D(d_buf), .SCD(si_full),
    .SCE(se_full), .Q(q_scan), .Q_N());
  sky130_fd_sc_hd__sdfxbp_1 ff_ideal (.CLK(clk_full), .D(d_buf),
    .SCD(si_buf), .SCE(se_buf), .Q(q_ideal), .Q_N());
  sky130_fd_sc_hd__sdfxbp_1 ff_prop (.CLK(clk_prop_full), .D(d_buf),
    .SCD(si_buf), .SCE(se_buf), .Q(q_prop), .Q_N());
endmodule
