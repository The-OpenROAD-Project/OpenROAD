module td1 (a1,
    a2,
    a3,
    a4, 
    a5,
    a6,
    clk,
    y1,
    y2);

 input a1;
 input a2;
 input a3;
 input a4;
 input a5;
 input a6;         
 input clk;
 output y1;
 output y2;

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

 pinswap  U4  (
	       .ip0(a6),	
	       .ip1(a4),
	       .ip2(a5),
	       .ip3(net1),		       
	       .op0(y2)
	       );
    
 BUF_X8 wire1 (.A(net1),
    .Z(y1));

endmodule // td1

module pinswap(
	       ip0,
	       ip1,
	       ip2,
	       ip3,
	       op0);
   input ip0;
   input ip1;
   input ip2;
   input ip3;
   output op0;

   AND4_X1 PS1 (
	    	.A1(ip0),	
		.A2(ip1),
		.A3(ip2),
		.A4(ip3),		       
		.ZN(op0)
	    );
   
	    
endmodule // pinswap

   
   
