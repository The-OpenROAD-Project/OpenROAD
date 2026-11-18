module top (clk, in1, out1, out2);
 input clk;
 input in1;
 output out1;
 output out2;

 wire r1q;
 wire oz;
 wire oz2;

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
endmodule

// Outer and its child Inner both have an instance _1_ and a net _013_.
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
