// BSD 3-Clause License
// 
// Copyright 2020 Lawrence T. Clark, Vinay Vashishtha, or Arizona State
// University
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Verilog for library /home/ltclark/ASAP7/LIB2/Liberate_2/Verilog/asap7sc7p5t_SIMPLE_RVT_TT_201020 created by Liberate 18.1.0.293 on Fri Nov 27 12:35:46 MST 2020 for SDF version 2.1

// type:  
`timescale 1ns/10ps
`celldefine
module AND2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	and (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND2x4_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	and (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND2x6_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	and (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND3x1_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	and (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND3x2_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	and (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND3x4_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	and (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND4x1_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	and (Y, A, B, C, D);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND4x2_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	and (Y, A, B, C, D);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND5x1_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	and (Y, A, B, C, D, E);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module AND5x2_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	and (Y, A, B, C, D, E);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module FAx1_ASAP7_75t_R (CON, SN, A, B, CI);
	output CON, SN;
	input A, B, CI;

	// Function
	wire A__bar, B__bar, CI__bar;
	wire int_fwire_0, int_fwire_1, int_fwire_2;
	wire int_fwire_3, int_fwire_4, int_fwire_5;
	wire int_fwire_6;

	not (CI__bar, CI);
	not (B__bar, B);
	and (int_fwire_0, B__bar, CI__bar);
	not (A__bar, A);
	and (int_fwire_1, A__bar, CI__bar);
	and (int_fwire_2, A__bar, B__bar);
	or (CON, int_fwire_2, int_fwire_1, int_fwire_0);
	and (int_fwire_3, A__bar, B__bar, CI__bar);
	and (int_fwire_4, A__bar, B, CI);
	and (int_fwire_5, A, B__bar, CI);
	and (int_fwire_6, A, B, CI__bar);
	or (SN, int_fwire_6, int_fwire_5, int_fwire_4, int_fwire_3);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module HAxp5_ASAP7_75t_R (CON, SN, A, B);
	output CON, SN;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (B__bar, B);
	not (A__bar, A);
	or (CON, A__bar, B__bar);
	and (int_fwire_0, A__bar, B__bar);
	and (int_fwire_1, A, B);
	or (SN, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module MAJIxp5_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;
	wire int_fwire_0, int_fwire_1, int_fwire_2;

	not (C__bar, C);
	not (B__bar, B);
	and (int_fwire_0, B__bar, C__bar);
	not (A__bar, A);
	and (int_fwire_1, A__bar, C__bar);
	and (int_fwire_2, A__bar, B__bar);
	or (Y, int_fwire_2, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module MAJx2_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire int_fwire_0, int_fwire_1, int_fwire_2;

	and (int_fwire_0, B, C);
	and (int_fwire_1, A, C);
	and (int_fwire_2, A, B);
	or (Y, int_fwire_2, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module MAJx3_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire int_fwire_0, int_fwire_1, int_fwire_2;

	and (int_fwire_0, B, C);
	and (int_fwire_1, A, C);
	and (int_fwire_2, A, B);
	or (Y, int_fwire_2, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2x1_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2x1p5_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2xp33_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2xp5_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND2xp67_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND3x1_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND3x2_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND3xp33_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND4xp25_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar;

	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar, D__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND4xp75_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar;

	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar, D__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NAND5xp2_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar, E__bar;

	not (E__bar, E);
	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	or (Y, A__bar, B__bar, C__bar, D__bar, E__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR2x1_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR2x1p5_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR2xp33_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR2xp67_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar;

	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR3x1_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR3x2_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR3xp33_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	wire A__bar, B__bar, C__bar;

	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR4xp25_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar;

	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar, D__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR4xp75_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar;

	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar, D__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module NOR5xp2_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	wire A__bar, B__bar, C__bar;
	wire D__bar, E__bar;

	not (E__bar, E);
	not (D__bar, D);
	not (C__bar, C);
	not (B__bar, B);
	not (A__bar, A);
	and (Y, A__bar, B__bar, C__bar, D__bar, E__bar);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	or (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR2x4_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	or (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR2x6_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	or (Y, A, B);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR3x1_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	or (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR3x2_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	or (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR3x4_ASAP7_75t_R (Y, A, B, C);
	output Y;
	input A, B, C;

	// Function
	or (Y, A, B, C);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR4x1_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	or (Y, A, B, C, D);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR4x2_ASAP7_75t_R (Y, A, B, C, D);
	output Y;
	input A, B, C, D;

	// Function
	or (Y, A, B, C, D);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR5x1_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	or (Y, A, B, C, D, E);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module OR5x2_ASAP7_75t_R (Y, A, B, C, D, E);
	output Y;
	input A, B, C, D, E;

	// Function
	or (Y, A, B, C, D, E);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module TIEHIx1_ASAP7_75t_R (H);
	output H;

	// Function
	buf (H, 1'b1);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module TIELOx1_ASAP7_75t_R (L);
	output L;

	// Function
	buf (L, 1'b0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XNOR2x1_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (B__bar, B);
	not (A__bar, A);
	and (int_fwire_0, A__bar, B__bar);
	and (int_fwire_1, A, B);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XNOR2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (B__bar, B);
	not (A__bar, A);
	and (int_fwire_0, A__bar, B__bar);
	and (int_fwire_1, A, B);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XNOR2xp5_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (B__bar, B);
	not (A__bar, A);
	and (int_fwire_0, A__bar, B__bar);
	and (int_fwire_1, A, B);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XOR2x1_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (A__bar, A);
	and (int_fwire_0, A__bar, B);
	not (B__bar, B);
	and (int_fwire_1, A, B__bar);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XOR2x2_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (A__bar, A);
	and (int_fwire_0, A__bar, B);
	not (B__bar, B);
	and (int_fwire_1, A, B__bar);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

// type:  
`timescale 1ns/10ps
`celldefine
module XOR2xp5_ASAP7_75t_R (Y, A, B);
	output Y;
	input A, B;

	// Function
	wire A__bar, B__bar, int_fwire_0;
	wire int_fwire_1;

	not (A__bar, A);
	and (int_fwire_0, A__bar, B);
	not (B__bar, B);
	and (int_fwire_1, A, B__bar);
	or (Y, int_fwire_1, int_fwire_0);

	
endmodule
`endcelldefine

