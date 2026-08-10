// Synthetic LVF POCV test netlist.
//
// Two independent register-to-register paths, each a 4-stage buffer chain:
//   HI path : launch_hi -> snl_bufhi x4 -> capture_hi   (large LVF sigma)
//   LO path : launch_lo -> snl_buflo x4 -> capture_lo   (small LVF sigma)
// Both chains use the SAME buffer mean tables, so the nominal (mean) path
// delay is identical; only the per-stage ocv_sigma_cell_* differs. Under
// `set_pocv_sigma -from_liberty` the HI chain must accumulate a larger arrival
// sigma (less optimistic statistical slack) than the LO chain.
module top (clk, in_hi, in_lo, out_hi, out_lo);
  input clk, in_hi, in_lo;
  output out_hi, out_lo;

  wire lhq, llq;            // launch flop outputs
  wire h1, h2, h3, h4;      // HI chain nets
  wire l1, l2, l3, l4;      // LO chain nets

  // launch flops
  snl_ffqx1 launch_hi (.D(in_hi), .CP(clk), .Q(lhq));
  snl_ffqx1 launch_lo (.D(in_lo), .CP(clk), .Q(llq));

  // HI buffer chain
  snl_bufhi uh1 (.A(lhq), .Z(h1));
  snl_bufhi uh2 (.A(h1),  .Z(h2));
  snl_bufhi uh3 (.A(h2),  .Z(h3));
  snl_bufhi uh4 (.A(h3),  .Z(h4));

  // LO buffer chain
  snl_buflo ul1 (.A(llq), .Z(l1));
  snl_buflo ul2 (.A(l1),  .Z(l2));
  snl_buflo ul3 (.A(l2),  .Z(l3));
  snl_buflo ul4 (.A(l3),  .Z(l4));

  // capture flops
  snl_ffqx1 capture_hi (.D(h4), .CP(clk), .Q(out_hi));
  snl_ffqx1 capture_lo (.D(l4), .CP(clk), .Q(out_lo));
endmodule // top
