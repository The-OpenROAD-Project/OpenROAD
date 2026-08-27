// TARGETS: bracket_underscore_alias, three_way_alias, bus_bit_vs_scalar
// CLUE: all three pre-images of odb::replaceBracketsWithUnderscores
// (util.cpp:31-52) are live in one module at once: bus bit data[5], escaped
// scalar \data[5] and plain scalar data_5_ all sanitize to "data_5_".  The
// read/link/write path must keep three distinct nets; existing cases carry only
// two of the three at a time.  Every bus bit is consumed so the boundary
// input set cannot shrink.
module top (data, y0, y1, y2);
   input [5:0] data;
   output y0, y1, y2;
   wire \data[5] ;
   wire data_5_;
   wire t0, t1, t2;
   INV_X1 g0 (.A(data[5]), .ZN(\data[5] ));
   BUF_X1 g1 (.A(\data[5] ), .Z(y0));
   INV_X1 g2 (.A(data[4]), .ZN(data_5_));
   BUF_X1 g3 (.A(data_5_), .Z(y1));
   AND2_X1 a0 (.A1(data[0]), .A2(data[1]), .ZN(t0));
   AND2_X1 a1 (.A1(data[2]), .A2(data[3]), .ZN(t1));
   AND2_X1 a2 (.A1(t0), .A2(t1), .ZN(t2));
   BUF_X1 g4 (.A(t2), .Z(y2));
endmodule
