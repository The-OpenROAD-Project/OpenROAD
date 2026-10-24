// Minimal asap7 netlist for prima_net_recycle.
// Only cells present in asap7/asap7_small.lib.gz (the in-tree CCS liberty).
//
// l0/l1/l2 exist to load n1 past 1.44fF, the lowest
// total_output_net_capacitance in the CCS tables.  Below that,
// PrimaDelayCalc::checkArgs() falls back to the table calculator and the
// stale-parasitic path is never reached.
module prima_net_recycle (clk, in, out);
  input clk;
  input in;
  output out;

  wire n0;
  wire n1;
  wire n2;
  wire d1;
  wire d2;

  DFFHQx4_ASAP7_75t_R r1 (.CLK(clk), .D(in), .Q(n0));
  INVx2_ASAP7_75t_R u1 (.A(n0), .Y(n1));
  INVx2_ASAP7_75t_R l0 (.A(n1), .Y(n2));
  INVx2_ASAP7_75t_R l1 (.A(n1), .Y(d1));
  INVx2_ASAP7_75t_R l2 (.A(n1), .Y(d2));
  DFFHQx4_ASAP7_75t_R r2 (.CLK(clk), .D(n2), .Q(out));
endmodule
