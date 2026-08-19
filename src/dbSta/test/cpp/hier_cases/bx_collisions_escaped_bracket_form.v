// TARGETS: bracket_underscore_alias, escaped_bracket_form
// CLUE: the sanitizer collapses an ESCAPED bracket pair the same way it
// collapses a plain one, so `\data\[5\] ` and the bus bit `data[5]` both map to
// `data_5_`. Three distinct source names, one sanitized string.
module top (data, y0, y1);
   input [5:0] data;
   output y0, y1;
   wire \data\[5\] ;
   INV_X1 g0 (.A(data[5]), .ZN(\data\[5\] ));
   BUF_X1 g1 (.A(\data\[5\] ), .Z(y0));
   BUF_X1 g2 (.A(data[5]), .Z(y1));
endmodule
