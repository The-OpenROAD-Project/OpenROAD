module max_chain_count(port1,
  clock,
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
  output10);
  input port1;
  input clock;
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


  // Rising edge flops
  sky130_fd_sc_hd__dfstp_1 ff1_clk1_rising(
    .Q(output1),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff2_clk1_rising(
    .Q(output2),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff3_clk1_rising(
    .Q(output3),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff4_clk1_rising(
    .Q(output4),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff5_clk1_rising(
    .Q(output5),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff6_clk1_rising(
    .Q(output6),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );
  sky130_fd_sc_hd__dfstp_1 ff7_clk1_rising(
    .Q(output7),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff8_clk1_rising(
    .Q(output8),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff9_clk1_rising(
    .Q(output9),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff10_clk1_rising(
    .Q(output10),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );


endmodule
