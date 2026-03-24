module top (in,
    out);
 input in;
 output out;

 wire n1;

 BUF_X4 new_buf1 (.A(n1),
    .Z(out));
endmodule
