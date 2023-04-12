module scan_architect(port1, clock1, clock2, output1, set_b);
  input port1;
  input clock1;
  input clock2;
  input set_b;
  output output1;

  // Rising edge flops
  sky130_fd_sc_hd__dfstp_1 ff1_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff2_clk2_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff3_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff4_clk2_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff5_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff6_clk2_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );
  sky130_fd_sc_hd__dfstp_1 ff7_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff8_clk2_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff9_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff10_clk2_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );
  
  // Falling edge flops
  sky130_fd_sc_hd__dfbbn_1 ff1_clk1_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff2_clk2_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff3_clk1_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff4_clk2_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff5_clk1_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff6_clk2_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );
  sky130_fd_sc_hd__dfbbn_1 ff7_clk1_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff8_clk2_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff9_clk1_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff10_clk2_falling(
    .Q(output1),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );


endmodule
