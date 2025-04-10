module clocked_macro ( );
  wire   in, clk, out;

  CLOCKED_MACRO U1 ( .I1(in), .CK(clk), .O1(out) ) ;

endmodule

