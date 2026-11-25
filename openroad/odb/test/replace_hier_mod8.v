module top (clk, in1, in2, out1, out2, out3, out4, out5);
 input clk;
 input in1;
 input in2;
 output out1;
 output out2;
 output out3;
 output out4;
 output out5;

 wire r1q;
 wire oz;
 wire oz2;
 wire oz3;
 wire rbq;
 wire x1;
 wire x2;

 DFF_X1 r1 (.CK(clk),
    .D(in1),
    .Q(r1q));
 Outer outer (.I(r1q), .O(oz));
 DFF_X1 r2 (.CK(clk),
    .D(oz),
    .Q(out1));
 Outer_new outer2 (.I(r1q), .O(oz2));
 DFF_X1 r3 (.CK(clk),
    .D(oz2),
    .Q(out2));
 Outer_new outer3 (.I(r1q), .O(oz3));
 DFF_X1 r4 (.CK(clk),
    .D(oz3),
    .Q(out3));

 // Port C is unconnected on u and connected on u2.
 DFF_X1 rb (.CK(clk),
    .D(in2),
    .Q(rbq));
 OldF u (.A(r1q), .C(), .X(x1));
 DFF_X1 r5 (.CK(clk),
    .D(x1),
    .Q(out4));
 NewF u2 (.A(r1q), .C(rbq), .X(x2));
 DFF_X1 r6 (.CK(clk),
    .D(x2),
    .Q(out5));
endmodule

// One child.
module Outer (I, O);
 input I;
 output O;

 wire _013_;
 wire _014_;

 BUF_X1 _1_ (.A(I),
    .Z(_013_));
 Inner inner (.I(_013_), .O(_014_));
 BUF_X1 _2_ (.A(_014_),
    .Z(O));
endmodule

// Two levels of hierarchy below it.
module Outer_new (I, O);
 input I;
 output O;

 wire _013_;
 wire _014_;
 wire _015_;

 INV_X1 _1_ (.A(I),
    .ZN(_013_));
 Mid mid (.I(_013_), .O(_014_));
 Inner inner (.I(_014_), .O(_015_));
 INV_X1 _2_ (.A(_015_),
    .ZN(O));
endmodule

module Mid (I, O);
 input I;
 output O;

 wire _013_;

 Inner inner (.I(I), .O(_013_));
 BUF_X1 _1_ (.A(_013_),
    .Z(O));
endmodule

module Inner (I, O);
 input I;
 output O;

 wire _013_;

 INV_X1 _1_ (.A(I),
    .ZN(_013_));
 INV_X1 _2_ (.A(_013_),
    .ZN(O));
endmodule

module OldF (A, C, X);
 input A;
 input C;
 output X;

 AND2_X1 g1 (.A1(A),
    .A2(C),
    .ZN(X));
endmodule

module NewF (A, C, X);
 input A;
 input C;
 output X;

 wire t;
 wire w;

 INV_X1 i (.A(A),
    .ZN(t));
 AND2_X1 g (.A1(t),
    .A2(C),
    .ZN(w));
 Gate gc (.A(w), .C(C), .X(X));
endmodule

// A child that port C of NewF reaches.
module Gate (A, C, X);
 input A;
 input C;
 output X;

 AND2_X1 g (.A1(A),
    .A2(C),
    .ZN(X));
endmodule
