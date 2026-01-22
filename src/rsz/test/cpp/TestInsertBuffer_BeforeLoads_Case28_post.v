module top (in);
 input in;

 wire net1;
 wire _019_;

 BUF_X1 drvr (.A(in),
    .Z(_019_));
 H1 h1 (._019__0(net1),
    .A(_019_));
 BUF_X1 load4 (.A(net1));
 BUF_X4 new_buf1 (.A(_019_),
    .Z(net1));
endmodule
module H1 (_019__0,
    A);
 input _019__0;
 input A;

 wire _019_;

 BUF_X1 load1 (.A(_019__0));
 BUF_X1 load2 (.A(_019_));
 BUF_X1 load3 (.A(A));
 XOR2_X1 xor_gate (.A(A),
    .B(A),
    .Z(_019_));
endmodule
