module scan_architect_no_mix_nangate45 (
    clk,
    in1,
    out);
 input clk;
 input in1;
 output out;

 wire n1;
 wire n2;
 wire n3;

 DFF_X1 ff1 (.D(in1),
    .CK(clk),
    .Q(n1));

 DFF_X1 ff2 (.D(n1),
    .CK(clk),
    .Q(n2));

 DFF_X1 ff3 (.D(n2),
    .CK(clk),
    .Q(n3));

 BUF_X1 buf1 (.A(n3),
    .Z(out));
endmodule
