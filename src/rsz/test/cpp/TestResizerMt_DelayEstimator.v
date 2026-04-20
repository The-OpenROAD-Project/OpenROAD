module top (clk,
    in,
    path_in,
    path_side,
    out0,
    out1,
    out2,
    path_out);
 input clk;
 input in;
 input path_in;
 input path_side;
 output out0;
 output out1;
 output out2;
 output path_out;

 wire n1;
 wire path_n0;
 wire path_n1;
 wire path_n2;
 wire path_n3;

 BUF_X1 target (.A(in),
    .Z(n1));
 BUF_X16 load0 (.A(n1),
    .Z(out0));
 BUF_X16 load1 (.A(n1),
    .Z(out1));
 BUF_X16 load2 (.A(n1),
    .Z(out2));

 BUF_X16 path_pre0 (.A(path_in),
    .Z(path_n0));
 BUF_X16 path_pre1 (.A(path_n0),
    .Z(path_n1));
 BUF_X1 path_side_buf (.A(path_side),
    .Z(path_n2));
 NAND2_X1 path_target (.A1(path_n1),
    .A2(path_n2),
    .ZN(path_n3));
 BUF_X16 path_load (.A(path_n3),
    .Z(path_out));
endmodule
