module top ();

 wire net1;
 wire ic_debug_addr_2;

 BUF_X1 buf1 (.A(ic_debug_addr_2),
    .Z(net1));
 swerv swerv_inst (.ic_debug_addr_2_0(net1),
    .ic_debug_addr_2(ic_debug_addr_2));
 BUF_X1 u_2884 (.A(net1));
 BUF_X1 u_2885 (.A(net1));
 BUF_X1 u_2924 (.A(net1));
 BUF_X1 u_2961 (.A(net1));
 BUF_X1 u_2999 (.A(net1));
 BUF_X1 u_3034 (.A(net1));
 BUF_X1 u_3041 (.A(net1));
 BUF_X1 u_3048 (.A(net1));
 BUF_X1 u_3055 (.A(net1));
endmodule
module dec_tlu_ctl (dec_tlu_ic_diag_pkt_2);
 output dec_tlu_ic_diag_pkt_2;


 BUF_X1 driver (.A(\swerv_inst/dec_tlu/zero_ ),
    .Z(dec_tlu_ic_diag_pkt_2));
endmodule
module ifu (ic_debug_addr_2_0,
    dec_tlu_ic_diag_pkt_2,
    ic_debug_addr_2);
 input ic_debug_addr_2_0;
 input dec_tlu_ic_diag_pkt_2;
 output ic_debug_addr_2;


 BUF_X1 u_11044 (.A(ic_debug_addr_2_0));
 BUF_X1 u_11045 (.A(ic_debug_addr_2_0));
 BUF_X1 u_11046 (.A(ic_debug_addr_2_0));
 BUF_X1 u_11047 (.A(ic_debug_addr_2_0));
 assign ic_debug_addr_2 = dec_tlu_ic_diag_pkt_2;
endmodule
module swerv (ic_debug_addr_2_0,
    ic_debug_addr_2);
 input ic_debug_addr_2_0;
 output ic_debug_addr_2;

 wire dec_tlu_ic_diag_pkt_2;

 dec_tlu_ctl dec_tlu (.dec_tlu_ic_diag_pkt_2(dec_tlu_ic_diag_pkt_2));
 ifu ifu (.ic_debug_addr_2_0(ic_debug_addr_2_0),
    .dec_tlu_ic_diag_pkt_2(dec_tlu_ic_diag_pkt_2),
    .ic_debug_addr_2(ic_debug_addr_2));
endmodule
