module sub_modules(port1, clock, output1, set_b);
  input port1;
  input clock;
  input set_b;
  output output1;

 shift_register_2b my_shift_register(port1, output1, clock);

endmodule

module shift_register_2b(from, to, clock);
  input from;
  output to;
  input clock;
  wire net1;

  sky130_fd_sc_hd__dfstp_1 ff1(
    .Q(net1),
    .D(from),
    .CLK(clock),
    .SET_B(set_b)
  );

  sky130_fd_sc_hd__dfstp_1 ff2(
    .Q(to),
    .D(net1),
    .CLK(clock),
    .SET_B(set_b)
  );
endmodule
