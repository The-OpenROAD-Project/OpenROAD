// Reproducer for the dbReadVerilog deep-descendant modBTerm
// false-attach bug
//
// The trigger pattern is:
//   - A non-top module M has a port named 'clk'.
//   - M instantiates a sub-module whose name is an ESCAPED Verilog
//     identifier containing '/' and '[]' characters.
//   - One such instance is wired to M.clk; another sibling is wired
//     to a different M port (here 'txclk').
//
// Inside Verilog2db::staToDb, when constructModNet processes the pin
// of the sibling instance:
//   - cur_inst's stripped local name doesn't match findModInst.
//   - cur_inst's full hierarchical path doesn't match findDbInst,
//     because the dbInst hash uses local names, not full paths.
//   - The reader then fell through to findModBTerm("clk"), which
//     name-matched M's own clk modBTerm and falsely connected it to
//     the sibling's modnet.
// Result: M's 'txclk' modnet ends up with both txclk and clk
// modBTerms attached, cross-aliasing the two external clocks at M's
// boundary (warning ORD-2059, or fatal ORD-2030 in some cases).

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
