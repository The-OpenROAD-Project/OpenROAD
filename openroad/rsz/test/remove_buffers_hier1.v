module sm(a, b, c);
 input a;
 output b;
 output c;
 wire n1, n2;
 INVx1_ASAP7_75t_R inv1(.A(a), .Y(n1));
 BUFx2_ASAP7_75t_R buf1(.A(n1), .Y(b));
 AND3x1_ASAP7_75t_R gate1(.A(n2), .B(b), .C(n2), .Y(c));
 TIEHIx1_ASAP7_75t_R tie1(.HI(n2));
endmodule

module top(i, j, k);
 input i;
 output j;
 output k;
 wire n1, n2, n3;
 sm m1(.a(n1), .b(n2), .c(n3));
 INVx1_ASAP7_75t_R inv1(.A(i), .Y(n1));
 INVx1_ASAP7_75t_R inv2(.A(n2), .Y(j));
 INVx1_ASAP7_75t_R inv3(.A(n3), .Y(k));
endmodule
