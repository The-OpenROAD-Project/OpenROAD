module one_cell(port1, clock, output1, set_b);
  input port1;
  input clock;
  input set_b;
  output output1;

  sky130_fd_sc_hd__dfstp_1 ff1(
    .Q(output1),
    .D(port1),
    .CLK(clock),
    .SET_B(set_b)
  );

endmodule
