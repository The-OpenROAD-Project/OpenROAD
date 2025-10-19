module AND2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);
endmodule // AND2_X1

module AND2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);
endmodule // AND2_X2

module AND2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 & A2);
endmodule // AND2_X4

module AND3_X1 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 & A2 & A3);
endmodule // AND3_X1

module AND3_X2 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 & A2 & A3);
endmodule // AND3_X2

module AND3_X4 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 & A2 & A3);
endmodule // AND3_X4

module AND4_X1 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 & A2 & A3 & A4);   
endmodule // AND4_X1

module AND4_X2 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 & A2 & A3 & A4);
endmodule // AND4_X2

module AND4_X4 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 & A2 & A3 & A4);
endmodule // AND4_X4

module AOI211_X1 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;  
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI211_X1

module AOI211_X2 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;  
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI211_X2

module AOI211_X4 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;  
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI211_X4

module AOI21_X1 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B; 
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule // AOI21_X1

module AOI21_X2 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B; 
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule // AOI21_X2

module AOI21_X4 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B; 
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule // AOI21_X4

module AOI221_X1 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B; 
   wire   C;  
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI221_X1

module AOI221_X2 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B; 
   wire   C;  
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI221_X2

module AOI221_X4 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B; 
   wire   C;  
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI221_X4

module AOI222_X1 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B; 
   wire   C;  
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI222_X1

module AOI222_X2 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B; 
   wire   C;  
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI222_X2

module AOI222_X4 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B; 
   wire   C;  
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign C = (C1 & C2);
   assign ZN = ~(A|B|C);
endmodule // AOI222_X4

module AOI22_X1 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule // AOI22_X1

module AOI22_X2 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule // AOI22_X2

module AOI22_X4 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1 & A2);
   assign B = (B1 & B2);
   assign ZN = ~(A|B);
endmodule

module BUF_X1 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X1

module BUF_X2 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X2

module BUF_X4 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X4

module BUF_X8 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X8

module BUF_X16 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X16

module BUF_X32 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // BUF_X32

module CLKBUF_X1 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X1

module CLKBUF_X2 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X2

module CLKBUF_X3 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X3

module CLKBUF_X4 (A, Z);
   input A;
   output Z;
   assign Z = (A);
endmodule // CLKBUF_X4

module DFF_X1 (CK, D, Q, QN);
   input CK;
   input D;
   output Q;
   output QN;   
   always @(posedge CK) begin
      Q <= D;
      QN <= ~D;
   end
endmodule // DFF_X1

module DFF_X2 (CK, D, Q, QN);
   input CK;
   input D;
   output Q;
   output QN;
   always @(posedge CK) begin
      Q <= D;
      QN <= ~D;
   end
endmodule // DFF_X2

module DFFR_X1 (CK, D, RN, Q, QN);
   input CK;
   input D;
   input RN;
   output Q;
   output QN;
   always @(posedge CK or negedge RN)
     if (RN==1'b0) 
       Q <= 0;
     else 
       Q <= D;   
   assign QN = ~Q;
endmodule // DFF_X1

module DFFR_X2 (CK, D, RN, Q, QN);
   input CK;
   input D;
   input RN;
   output Q;
   output QN;
   always @(posedge CK or negedge RN)
     if (RN==1'b0) 
       Q <= 0;
     else 
       Q <= D;   
   assign QN = ~Q;
endmodule // DFF_X2

module DFFS_X1 (D, SN, CK, Q, QN);
   input D;
   input SN;
   input CK;
   output Q;
   output QN;
   always @(posedge CK or negedge SN)
     if (SN==1'b0) 
       Q <= 1;
     else 
       Q <= D;   
   assign QN = ~Q;
endmodule

module DFFS_X2 (D, SN, CK, Q, QN);
   input D;
   input SN;
   input CK;
   output Q;
   output QN;
   always @(posedge CK or negedge SN)
     if (SN==1'b0) 
       Q <= 1;
     else 
       Q <= D;   
   assign QN = ~Q;
endmodule

module HA_X1 (A, B, CO, S);
   input A;
   input B;
   output CO;
   output S;
   assign S = A ^ B;  // Dataflow expression for sum
   assign CO = A & B;  // Dataflow expression for carry
endmodule // HA_X1

module FA_X1 (A, B, CI, CO, S);
   input A;
   input B;
   input CI;
   output CO;
   output S;
   assign S  = (A ^ B) ^ CI;
   assign CO = (A & B) | (A & CI) | (B & CI);
endmodule

module INV_X1 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X1

module INV_X16 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X16

module INV_X2 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X2

module INV_X32 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X32

module INV_X4 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X4

module INV_X8 (A, ZN);
   input A;
   output ZN;
   assign ZN = ~A;
endmodule // INV_X8

module LOGIC0_X1 (Z);
   output Z;
   assign Z = 0;
endmodule // LOGIC0_X1

module LOGIC1_X1 (Z);
   output Z;
   assign Z = 1;
endmodule // LOGIC1_X1

module MUX2_X1 (A, B, S, Z);
   input A, B, S;
   output Z;
   // assign one of the inputs to the output based upon select line input
   assign Z = S ? B : A;
endmodule // MUX2_X1

module MUX2_X2 (A, B, S, Z);
   input A, B, S;
   output Z;
   // assign one of the inputs to the output based upon select line input
   assign Z = S ? B : A;
endmodule // MUX2_X2

module NAND2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 & A2);   
endmodule // NAND2_X1

module NAND2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 & A2);   
endmodule // NAND2_X2

module NAND2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 & A2);   
endmodule // NAND2_X4

module NAND3_X1 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 & A2 & A3);
endmodule // NAND3_X1

module NAND3_X2 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 & A2 & A3);
endmodule // NAND3_X2

module NAND3_X4 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 & A2 & A3);
endmodule // NAND3_X4

module NAND4_X1 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = ~(A1 & A2 & A3 & A4);
endmodule // NAND4_X1

module NAND4_X2 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = ~(A1 & A2 & A3 & A4);
endmodule // NAND4_X2

module NAND4_X4 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;   
   assign ZN = ~(A1 & A2 & A3 & A4);
endmodule // NAND4_X4

module NOR2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 | A2);
endmodule // NOR2_X1

module NOR2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~ (A1 | A2);
endmodule // NOR2_X2

module NOR2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = ~(A1 | A2);
endmodule // NOR2_X4

module NOR3_X1 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 | A2 | A3);
endmodule // NOR3_X1

module NOR3_X2 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 | A2 | A3);
endmodule // NOR3_X2

module NOR3_X4 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = ~(A1 | A2 | A3);   
endmodule // NOR3_X4

module NOR4_X1 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = ~(A1 | A2 | A3 | A4);
endmodule // NOR4_X1

module NOR4_X2 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = ~(A1 | A2 | A3 | A4);
endmodule // NOR4_X2

module NOR4_X4 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = ~(A1 | A2 | A3 | A4);
endmodule // NOR4_X4

module OAI211_X1 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI211_X1

module OAI211_X2 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI211_X2

module OAI211_X4 (A, B, C1, C2, ZN);
   input A;
   input B;
   input C1;
   input C2;
   output ZN;
   wire   C;
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI211_X4

module OAI21_X1 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B;   
   assign B = (B1|B2);
   assign ZN = ~(A & B);
endmodule // OAI21_X1

module OAI21_X2 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B;   
   assign B = (B1|B2);
   assign ZN = ~(A & B);   
endmodule // OAI21_X2

module OAI21_X4 (A, B1, B2, ZN);
   input A;
   input B1;
   input B2;
   output ZN;
   wire   B;   
   assign B = (B1|B2);
   assign ZN = ~(A & B);
endmodule // OAI21_X4

module OAI221_X1 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B;
   wire   C;
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI221_X1

module OAI221_X2 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B;
   wire   C;
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI221_X2

module OAI221_X4 (A, B1, B2, C1, C2, ZN);
   input A;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   B;
   wire   C;
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI221_X4

module OAI222_X1 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B;
   wire   C;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI222_X1

module OAI222_X2 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B;
   wire   C;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI222_X2

module OAI222_X4 (A1, A2, B1, B2, C1, C2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   input C1;
   input C2;
   output ZN;
   wire   A;
   wire   B;
   wire   C;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign C = (C1|C2);
   assign ZN = ~(A & B & C);
endmodule // OAI222_X4

module OAI22_X1 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign ZN = ~(A & B);   
endmodule // OAI22_X1

module OAI22_X2 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign ZN = ~(A & B);
endmodule // OAI22_X2

module OAI22_X4 (A1, A2, B1, B2, ZN);
   input A1;
   input A2;
   input B1;
   input B2;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1|A2);
   assign B = (B1|B2);
   assign ZN = ~(A & B);
endmodule // OAI22_X4

module OAI33_X1 (A1, A2, A3, B1, B2, B3, ZN);
   input A1;
   input A2;
   input A3;
   input B1;
   input B2;
   input B3;
   output ZN;
   wire   A;
   wire   B;
   assign A = (A1|A2|A3);
   assign B = (B1|B2|B3);
   assign ZN = ~(A & B);
endmodule // OAI33_X1

module OR2_X1 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 | A2);
endmodule // OR2_X1

module OR2_X2 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 | A2);
endmodule // OR2_X2

module OR2_X4 (A1, A2, ZN);
   input A1;
   input A2;
   output ZN;
   assign ZN = (A1 | A2);
endmodule // OR2_X4

module OR3_X1 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 | A2 | A3);
endmodule // OR3_X1

module OR3_X2 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 | A2 | A3);
endmodule // OR3_X2

module OR3_X4 (A1, A2, A3, ZN);
   input A1;
   input A2;
   input A3;
   output ZN;
   assign ZN = (A1 | A2 | A3);      
endmodule // OR3_X4

module OR4_X1 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 | A2 | A3 | A4);   
endmodule // OR4_X1

module OR4_X2 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 | A2 | A3 | A4);
endmodule // OR4_X2

module OR4_X4 (A1, A2, A3, A4, ZN);
   input A1;
   input A2;
   input A3;
   input A4;
   output ZN;
   assign ZN = (A1 | A2 | A3 | A4);   
endmodule // OR4_X4

module XNOR2_X1 (A, B, ZN);
   input A;
   input B;
   output ZN;
   assign ZN = (A ^~ B);
endmodule // XNOR_X1

module XNOR2_X2 (A, B, ZN);
   input A;
   input B;
   output ZN;
   assign ZN = (A ^~ B);
endmodule // XNOR_X2

module XNOR2_X4 (A, B, ZN);
   input A;
   input B;
   output ZN;
   assign ZN = (A ^~ B);
endmodule // XNOR_X4

module XOR2_X1 (A, B, Z);
   input A;
   input B;
   output Z;
   assign Z = (A ^ B);
endmodule // XOR2_X1

module XOR2_X2 (A, B, Z);
   input A;
   input B;
   output Z;
   assign Z = (A ^ B);
endmodule // XOR2_X2


