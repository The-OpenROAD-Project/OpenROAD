module scan_architect(port1,
  clock1,
  clock2,
  set_b,
  output1,
  output2,
  output3,
  output4,
  output5,
  output6,
  output7,
  output8,
  output9,
  output10,
  output11,
  output12,
  output13,
  output14,
  output15,
  output16,
  output17,
  output18,
  output19,
  output20);
  input port1;
  input clock1;
  input clock2;
  input set_b;
  output output1;
  output output2;
  output output3;
  output output4;
  output output5;
  output output6;
  output output7;
  output output8;
  output output9;
  output output10;
  output output11;
  output output12;
  output output13;
  output output14;
  output output15;
  output output16;
  output output17;
  output output18;
  output output19;
  output output20;

  // Rising edge flops
  sky130_fd_sc_hd__dfstp_1 ff1_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff2_clk2_rising(
    .Q(output2),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff3_clk1_rising(
    .Q(output3),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff4_clk2_rising(
    .Q(output4),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff5_clk1_rising(
    .Q(output5),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff6_clk2_rising(
    .Q(output6),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );
  sky130_fd_sc_hd__dfstp_1 ff7_clk1_rising(
    .Q(output7),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff8_clk2_rising(
    .Q(output8),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff9_clk1_rising(
    .Q(output9),
    .D(port1),
    .CLK(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff10_clk2_rising(
    .Q(output10),
    .D(port1),
    .CLK(clock2),
    .SET_B(set_b)
  );
  
  // Falling edge flops
  sky130_fd_sc_hd__dfbbn_1 ff1_clk1_falling(
    .Q(output11),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff2_clk2_falling(
    .Q(output12),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff3_clk1_falling(
    .Q(output13),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff4_clk2_falling(
    .Q(output14),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff5_clk1_falling(
    .Q(output15),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff6_clk2_falling(
    .Q(output16),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );
  sky130_fd_sc_hd__dfbbn_1 ff7_clk1_falling(
    .Q(output17),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff8_clk2_falling(
    .Q(output18),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff9_clk1_falling(
    .Q(output19),
    .D(port1),
    .CLK_N(clock1),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfbbn_1 ff10_clk2_falling(
    .Q(output20),
    .D(port1),
    .CLK_N(clock2),
    .SET_B(set_b)
  );


endmodule
