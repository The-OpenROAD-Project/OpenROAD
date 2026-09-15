// Reproducer for the dbReadVerilog deep-descendant modBTerm
// false-attach bug

module sub_with_clk (input clk, input d, output zo);
  DFF_X1 u_ff (.CK(clk), .D(d), .Q(zo));
endmodule

module mid (
    input  clk,
    input  txclk,
    input  d,
    output zo1,
    output zo2
);
  sub_with_clk \iclkdiv/gen_phases[0].iclk (.clk(clk),
                                            .d(d),
                                            .zo(zo1));
  sub_with_clk \iclkdiv/gen_phases[1].iclk (.clk(txclk),
                                            .d(d),
                                            .zo(zo2));
endmodule

module top (
    input  clk_a,
    input  clk_b,
    input  d_in,
    output out1,
    output out2
);
  mid u_mid (.clk(clk_a), .txclk(clk_b), .d(d_in), .zo1(out1), .zo2(out2));
endmodule
