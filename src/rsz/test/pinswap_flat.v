module td1 (a1,
    a2,
    a3,
    a4, 
    a5,
    a6,
    clk,
    y1,
    y2);

   input clk;
   output y1;
   output y2;

   input a1;
   input a2;
   input a3;
   input a4;
   input a5;
   input a6;
   
 wire n1;
 wire net1;
 wire net2;
 wire net3;

 NAND2_X1 U3 (.A1(a1),
	      .A2(a2),
	      .ZN(n1));
   
 AND2_X1 U5 (.A1(n1),
    .A2(a3),
    .ZN(net1));
   
 AND4_X1 U4  (
		       .A1(a6),	
		       .A2(a4),
		       .A3(a5),
		       .A4(net1),		       
		       .ZN(y2)
		       );
    
 BUF_X8 wire1 (.A(net1),
    .Z(y1));

endmodule
