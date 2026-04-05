module top (top_in);
 input top_in;

  wire w_hi;
 wire w_lo;

 HIER1 h1 (.H_IN(w_hi), .L_IN(w_lo));
 TIEHIx1_ASAP7_75t_R tie_hi (.H(w_hi));
 TIELOx1_ASAP7_75t_R tie_lo (.L(w_lo));
endmodule
module ALU_module_hi (A);
 input A;


 BUFx2_ASAP7_75t_R load1 (.A(A));
 BUFx2_ASAP7_75t_R load2 (.A(A));
endmodule
module ALU_module_lo (A);
 input A;


 BUFx2_ASAP7_75t_R load1 (.A(A));
 BUFx2_ASAP7_75t_R load2 (.A(A));
endmodule
module HIER1 (H_IN, L_IN);
 input H_IN;
 input L_IN;

 wire net;

 ALU_module_hi alu_hi (.A(H_IN));
 ALU_module_lo alu_lo (.A(L_IN));
 BUFx2_ASAP7_75t_R dummy (.A(net));
endmodule
