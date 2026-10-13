module top (a,
    b,
    out);
 input a;
 input b;
 output out;

 wire mid;

 sink_mod u_sink (.sink_i(mid),
    .sink_b(b),
    .sink_o(out));
 wrap_mod u_wrap (.wrap_a(a),
    .wrap_b(b),
    .wrap_o(mid));
endmodule
module blk_mod (blk_a,
    blk_b,
    blk_o);
 input blk_a;
 input blk_b;
 output blk_o;

 wire drv_to_ft;

 drv_mod u_drv (.drv_a(blk_a),
    .drv_b(blk_b),
    .drv_o(drv_to_ft));
 ft_mod u_ft_mod (.ft_i(drv_to_ft),
    .ft_o(blk_o));
endmodule
module drv_mod (drv_a,
    drv_b,
    drv_o);
 input drv_a;
 input drv_b;
 output drv_o;


 NOR2_X1 g_drv (.A1(drv_a),
    .A2(drv_b),
    .ZN(drv_o));
endmodule
module ft_mod (ft_i,
    ft_o);
 input ft_i;
 output ft_o;


 assign ft_o = ft_i;
endmodule
module sink_mod (sink_i,
    sink_b,
    sink_o);
 input sink_i;
 input sink_b;
 output sink_o;


 OR2_X1 g_sink (.A1(sink_i),
    .A2(sink_b),
    .ZN(sink_o));
endmodule
module wrap_mod (wrap_a,
    wrap_b,
    wrap_o);
 input wrap_a;
 input wrap_b;
 output wrap_o;


 blk_mod u_blk (.blk_a(wrap_a),
    .blk_b(wrap_b),
    .blk_o(wrap_o));
endmodule
