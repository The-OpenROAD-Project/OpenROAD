// repair_power tests: a chain p0..p5 of LVT AND2 cells from a to oend.
// Each p<k> also has its own side input s<k> and own output o<k>
// through a typical-VT buffer.  r0/r1 drive r_out; f0 drives f_out.
module top (clk, a, s0, s1, s2, s3, s4, s5, r_in, f_in, o0, o1, o2, o3, o4, o5, oend, r_out, f_out);
 input clk;
 input a;
 input s0;
 input s1;
 input s2;
 input s3;
 input s4;
 input s5;
 input r_in;
 input f_in;
 output o0;
 output o1;
 output o2;
 output o3;
 output o4;
 output o5;
 output oend;
 output r_out;
 output f_out;
 wire c0;
 wire c1;
 wire c2;
 wire c3;
 wire c4;
 wire c5;
 wire nr;

 AND2_X1_L p0 (.A1(a), .A2(s0), .ZN(c0));
 BUF_X4 q0 (.A(c0), .Z(o0));
 AND2_X1_L p1 (.A1(c0), .A2(s1), .ZN(c1));
 BUF_X4 q1 (.A(c1), .Z(o1));
 AND2_X1_L p2 (.A1(c1), .A2(s2), .ZN(c2));
 BUF_X4 q2 (.A(c2), .Z(o2));
 AND2_X1_L p3 (.A1(c2), .A2(s3), .ZN(c3));
 BUF_X4 q3 (.A(c3), .Z(o3));
 AND2_X1_L p4 (.A1(c3), .A2(s4), .ZN(c4));
 BUF_X4 q4 (.A(c4), .Z(o4));
 AND2_X1_L p5 (.A1(c4), .A2(s5), .ZN(c5));
 BUF_X4 q5 (.A(c5), .Z(o5));
 BUF_X4 pe (.A(c5), .Z(oend));
 BUF_X1_L r0 (.A(r_in), .Z(nr));
 BUF_X1_L r1 (.A(nr), .Z(r_out));
 BUF_X1_L f0 (.A(f_in), .Z(f_out));
endmodule
