/*
 Hierarchical verilog version of resize test 1
 */
module top (
            input  in1,
            input  in2,
            input  clk1,
            input  clk2,
            input  clk3,
            output out);
   u1 u1 (.in1(in1),
          .in2(in2),
          .clk1(clk1),
          .clk2(clk2),
          .clk3(clk3),
          .out(out));
endmodule // top

module u1 (in1,
    in2,
    clk1,
    clk2,
    clk3,
    out);
          
 input in1;
 input in2;
 input clk1;
 input clk2;
 input clk3;
 output out;
 wire r1q;
 wire r2q;
 wire u1z;
 wire u2z;
 AND2_X1 u2 (.A1(r1q),
    .A2(u1z),
    .ZN(u2z));
 BUF_X1 u1 (.A(r2q),
    .Z(u1z));
          
 DFF_X1 r3 (.D(u2z),
    .CK(clk3),
    .Q(out));
          
 DFF_X1 r2 (.D(in2),
    .CK(clk2),
    .Q(r2q));
 DFF_X1 r1 (.D(in1),
    .CK(clk1),
    .Q(r1q));
endmodule
