module dec_tlu_ctl (output dec_tlu_ic_diag_pkt_2);
  BUF_X1 driver (.A(1'b0), .Z(dec_tlu_ic_diag_pkt_2));
endmodule

module ifu (input dec_tlu_ic_diag_pkt_2, output ic_debug_addr_2);
  BUF_X1 u_11044 (.A(dec_tlu_ic_diag_pkt_2), .Z());
  BUF_X1 u_11045 (.A(dec_tlu_ic_diag_pkt_2), .Z());
  BUF_X1 u_11046 (.A(dec_tlu_ic_diag_pkt_2), .Z());
  BUF_X1 u_11047 (.A(dec_tlu_ic_diag_pkt_2), .Z());
  
  assign ic_debug_addr_2 = dec_tlu_ic_diag_pkt_2;
endmodule

module swerv (output ic_debug_addr_2);
  wire dec_tlu_ic_diag_pkt_2;
  
  dec_tlu_ctl dec_tlu (.dec_tlu_ic_diag_pkt_2(dec_tlu_ic_diag_pkt_2));
  
  ifu ifu (.dec_tlu_ic_diag_pkt_2(dec_tlu_ic_diag_pkt_2), .ic_debug_addr_2(ic_debug_addr_2));
endmodule

module top ();
  wire ic_debug_addr_2;
  
  swerv swerv_inst (.ic_debug_addr_2(ic_debug_addr_2));
  
  BUF_X1 u_2884 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_2885 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_2924 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_2961 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_2999 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_3034 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_3041 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_3048 (.A(ic_debug_addr_2), .Z());
  BUF_X1 u_3055 (.A(ic_debug_addr_2), .Z());
endmodule
