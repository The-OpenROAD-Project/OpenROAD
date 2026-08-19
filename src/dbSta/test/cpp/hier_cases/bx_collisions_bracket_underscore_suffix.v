// TARGETS: bracket_underscore_alias, uniquifier_suffix_taken
// CLUE: when the sanitized name `data_5_` is taken, the uniquifier's next
// candidate is `data_5__2` -- which this design also already owns. Both, plus
// the bus bit they derive from, must stay distinct.
module top (data, y0, y1, y2);
   input [5:0] data;
   output y0, y1, y2;
   wire \data_5_ ;
   wire \data_5__2 ;
   INV_X1 g0 (.A(data[5]), .ZN(\data_5_ ));
   INV_X1 g1 (.A(data[4]), .ZN(\data_5__2 ));
   BUF_X1 g2 (.A(\data_5_ ), .Z(y0));
   BUF_X1 g3 (.A(\data_5__2 ), .Z(y1));
   BUF_X1 g4 (.A(data[5]), .Z(y2));
endmodule
