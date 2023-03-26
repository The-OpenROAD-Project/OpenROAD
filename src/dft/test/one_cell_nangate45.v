module one_cell(port1, clock, output1, set_b);
  input port1;
  input clock;
  input set_b;
  output output1;

  DFF_X1 ff1(
    .D(port1),
    .Q(output1),
    .CL(clock)
  );

endmodule
