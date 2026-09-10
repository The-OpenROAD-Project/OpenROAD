// TARGETS: bracket_underscore_alias, bus_bit_vs_scalar
// CLUE: odb::replaceBracketsWithUnderscores maps `data[5]` to `data_5_`, and a
// design may already own a scalar of exactly that name. Both must survive the
// round trip as distinct objects; if a writer or reader ever routes a name
// through that sanitizer, the bus bit and the scalar alias.
module top (data, y0, y1);
   input [5:0] data;
   output y0, y1;
   wire \data_5_ ;
   INV_X1 g0 (.A(data[5]), .ZN(\data_5_ ));
   BUF_X1 g1 (.A(\data_5_ ), .Z(y0));
   BUF_X1 g2 (.A(data[5]), .Z(y1));
endmodule
