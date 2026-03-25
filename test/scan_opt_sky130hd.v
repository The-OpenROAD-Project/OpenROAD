module scan_opt_test(
  input port1,
  input port2,
  input clk,
  input set_b
);

  sky130_fd_sc_hd__dfstp_1 ff1  (.Q(),  .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff2  (.Q(),  .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff3  (.Q(),  .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff4  (.Q(),  .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff5  (.Q(),  .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff6  (.Q(),  .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff7  (.Q(),  .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff8  (.Q(),  .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff9  (.Q(),  .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff10 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff11 (.Q(), .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff12 (.Q(), .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff13 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff14 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff15 (.Q(), .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff16 (.Q(), .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff17 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff18 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff19 (.Q(), .D(port1), .CLK(clk), .SET_B(set_b));
  sky130_fd_sc_hd__dfstp_1 ff20 (.Q(), .D(port2), .CLK(clk), .SET_B(set_b));

endmodule
