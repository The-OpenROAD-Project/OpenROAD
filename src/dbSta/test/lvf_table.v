// Table-indexed LVF POCV test netlist. OpenROAD-fork: LVF-full
//
// Two independent register-to-register paths, each a single snl_bufvar stage.
// snl_bufvar's ocv_sigma_cell_* tables are 2-D (slew x load), with sigma that
// GROWS with output load. The two bufvar outputs are loaded differently (via
// set_load in lvf_table.tcl): the SMALL-load path samples the low-load corner
// of the sigma table, the LARGE-load path samples the high-load corner. Under
// `set_pocv_sigma -from_liberty` the large-load arc must therefore accumulate a
// LARGER arrival sigma than the small-load arc -- proving the library-driven
// POCV reads sigma as a TABLE LOOKUP (load-dependent), not a scalar constant.
module top (clk, in_sm, in_lg, out_sm, out_lg);
  input clk, in_sm, in_lg;
  output out_sm, out_lg;

  wire lsm, llg;     // launch flop outputs
  wire nsm, nlg;     // bufvar output nets (loaded differently)

  // launch flops
  snl_ffqx1 launch_sm (.D(in_sm), .CP(clk), .Q(lsm));
  snl_ffqx1 launch_lg (.D(in_lg), .CP(clk), .Q(llg));

  // single table-indexed-LVF buffer per path
  snl_bufvar usm (.A(lsm), .Z(nsm));   // small output load
  snl_bufvar ulg (.A(llg), .Z(nlg));   // large output load

  // capture flops
  snl_ffqx1 capture_sm (.D(nsm), .CP(clk), .Q(out_sm));
  snl_ffqx1 capture_lg (.D(nlg), .CP(clk), .Q(out_lg));
endmodule // top
