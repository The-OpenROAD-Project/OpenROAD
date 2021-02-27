module gcd_mem3 (
	clk, 
	req_msg, 
	req_rdy, 
	req_val, 
	reset, 
	resp_msg, 
	resp_rdy, 
	resp_val, 
	mem_out0, 
	mem_out1, 
	mem_out2);
   input clk;
   input [31:0] req_msg;
   output req_rdy;
   input req_val;
   input reset;
   output [15:0] resp_msg;
   input resp_rdy;
   output resp_val;
   output [6:0] mem_out0;
   output [6:0] mem_out1;
   output [6:0] mem_out2;
   wire [6:0]   data;
   
   // mem0/rd_out -> r1* -> r2* -> r3* -> mem1/wd_in
   DFF_X1 r10 (.D(mem_out0[0]),
	.CK(clk),
	.Q(l1[0]));
   DFF_X1 r11 (.D(mem_out0[1]),
	.CK(clk),
	.Q(l1[1]));
   DFF_X1 r12 (.D(mem_out0[2]),
	.CK(clk),
	.Q(l1[2]));
   DFF_X1 r13 (.D(mem_out0[3]),
	.CK(clk),
	.Q(l1[3]));
   DFF_X1 r14 (.D(mem_out0[4]),
	.CK(clk),
	.Q(l1[4]));
   DFF_X1 r15 (.D(mem_out0[5]),
	.CK(clk),
	.Q(l1[5]));
   DFF_X1 r16 (.D(mem_out0[6]),
	.CK(clk),
	.Q(l1[6]));

   DFF_X1 r20 (.D(l1[0]),
	.CK(clk),
	.Q(l2[0]));
   DFF_X1 r21 (.D(l1[1]),
	.CK(clk),
	.Q(l2[1]));
   DFF_X1 r22 (.D(l1[2]),
	.CK(clk),
	.Q(l2[2]));
   DFF_X1 r23 (.D(l1[3]),
	.CK(clk),
	.Q(l2[3]));
   DFF_X1 r24 (.D(l1[4]),
	.CK(clk),
	.Q(l2[4]));
   DFF_X1 r25 (.D(l1[5]),
	.CK(clk),
	.Q(l2[5]));
   DFF_X1 r26 (.D(l1[6]),
	.CK(clk),
	.Q(l2[6]));
   
   DFF_X1 r30 (.D(l2[0]),
	.CK(clk),
	.Q(l3[0]));
   DFF_X1 r31 (.D(l2[1]),
	.CK(clk),
	.Q(l3[1]));
   DFF_X1 r32 (.D(l2[2]),
	.CK(clk),
	.Q(l3[2]));
   DFF_X1 r33 (.D(l2[3]),
	.CK(clk),
	.Q(l3[3]));
   DFF_X1 r34 (.D(l2[4]),
	.CK(clk),
	.Q(l3[4]));
   DFF_X1 r35 (.D(l2[5]),
	.CK(clk),
	.Q(l3[5]));
   DFF_X1 r36 (.D(l2[6]),
	.CK(clk),
	.Q(l3[6]));

   fakeram45_64x7 mem0 (.clk(clk),
	.rd_out(mem_out0),
	.we_in(_006_),
	.ce_in(_007_),
	.addr_in({ _008_,
		_009_,
		_010_,
		_011_,
		_012_,
		_013_ }),
	.wd_in({ _014_,
		_015_,
		_016_,
		_017_,
		_018_,
		_019_,
		_020_ }),
	.w_mask_in({ _021_,
		_076_,
		_077_,
		_078_,
		_079_,
		_080_,
		_081_ }));
   fakeram45_64x7 mem1 (.clk(clk),
	.rd_out(mem_out1),
	.we_in(_090_),
	.ce_in(_091_),
	.addr_in({ _092_,
		_093_,
		_094_,
		_095_,
		_096_,
		_097_ }),
	.wd_in(l2[6:0]),
	.w_mask_in({ _105_,
		_106_,
		_107_,
		_054_,
		_055_,
		_056_,
		_003_ }));
   fakeram45_64x7 mem2 (.clk(clk),
	.rd_out(mem_out2),
	.we_in(_012_),
	.ce_in(_013_),
	.addr_in({ _014_,
		_015_,
		_016_,
		_017_,
		_018_,
		_019_ }),
	.wd_in({ _020_,
		_021_,
		_076_,
		_077_,
		_078_,
		_079_,
		_080_ }),
	.w_mask_in({ _081_,
		_082_,
		_083_,
		_084_,
		_085_,
		_086_,
		_087_ }));
endmodule
   
