(* noblackbox *) module sky130_ef_sc_hd__decap_12 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_ef_sc_hd__fakediode_2 (DIODE);


    input DIODE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__diode base (
        .DIODE(DIODE)
    );


endmodule
(* noblackbox *) module sky130_ef_sc_hd__fill_8 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;


endmodule
(* noblackbox *) module sky130_ef_sc_hd__fill_12 ();


    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fill base (
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dff$NSR (Q,SET,RESET,CLK_N,D);
output Q;
input SET;
input RESET;
input CLK_N;
input D;
reg Q;
wire AD = SET;
wire AL = SET | RESET;
always @(negedge CLK_N or posedge AL)
  if (AL) Q <= AD;
  else Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dff$P (Q,D,CLK);
output Q;
input D;
input CLK;
reg Q;
always @(posedge CLK) Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dff$PR (Q,D,CLK,RESET);
output Q;
input D;
input CLK;
input RESET;
reg Q;
always @(posedge CLK or posedge RESET)
  if (RESET) Q <= 1'b0;
  else Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dff$PS (Q,D,CLK,SET);
output Q;
input D;
input CLK;
input SET;
reg Q;
always @(posedge CLK or posedge SET)
  if (SET) Q <= 1'b1;
  else Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dlatch$lP (Q,D,GATE);
output Q;
input D;
input GATE;
reg Q;
always @(GATE or D)
  if (GATE) Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dlatch$P (Q,D,GATE);
output Q;
input D;
input GATE;
reg Q;
always @(GATE or D)
  if (GATE) Q <= D;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_dlatch$PR (Q,D,GATE,RESET);
output Q;
input D;
input GATE;
input RESET;
reg Q;
wire AG = GATE | RESET;
wire AD = (~RESET) & D;
always @(AG or AD)
  if (AG) Q <= AD;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_mux_2to1 (X,A0,A1,S);
output X;
reg X;
input A0;
input A1;
input S;
always @* casez ({A0,A1,S})
  3'b00?: {X} = 1'b0;
  3'b11?: {X} = 1'b1;
  3'b0?0: {X} = 1'b0;
  3'b1?0: {X} = 1'b1;
  3'b?01: {X} = 1'b0;
  3'b?11: {X} = 1'b1;
endcase;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_mux_2to1_N (Y,A0,A1,S);
output Y;
reg Y;
input A0;
input A1;
input S;
always @* casez ({A0,A1,S})
  3'b0?0: {Y} = 1'b1;
  3'b1?0: {Y} = 1'b0;
  3'b?01: {Y} = 1'b1;
  3'b?11: {Y} = 1'b0;
  3'b00?: {Y} = 1'b1;
  3'b11?: {Y} = 1'b0;
endcase;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_mux_4to2 (X,A0,A1,A2,A3,S0,S1);
output X;
reg X;
input A0;
input A1;
input A2;
input A3;
input S0;
input S1;
always @* casez ({A0,A1,A2,A3,S0,S1})
  6'b0???00: {X} = 1'b0;
  6'b1???00: {X} = 1'b1;
  6'b?0??10: {X} = 1'b0;
  6'b?1??10: {X} = 1'b1;
  6'b??0?01: {X} = 1'b0;
  6'b??1?01: {X} = 1'b1;
  6'b???011: {X} = 1'b0;
  6'b???111: {X} = 1'b1;
  6'b0000??: {X} = 1'b0;
  6'b1111??: {X} = 1'b1;
  6'b00???0: {X} = 1'b0;
  6'b11???0: {X} = 1'b1;
  6'b??00?1: {X} = 1'b0;
  6'b??11?1: {X} = 1'b1;
  6'b0?0?0?: {X} = 1'b0;
  6'b1?1?0?: {X} = 1'b1;
  6'b?0?01?: {X} = 1'b0;
  6'b?1?11?: {X} = 1'b1;
endcase;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_pwrgood$l_pp$G (UDP_OUT,UDP_IN,VGND);
output UDP_OUT;
input UDP_IN;
input VGND;
assign UDP_OUT = UDP_IN;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_pwrgood_pp$G (UDP_OUT,UDP_IN,VGND);
output UDP_OUT;
input UDP_IN;
input VGND;
assign UDP_OUT = UDP_IN;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__udp_pwrgood_pp$P (UDP_OUT,UDP_IN,VPWR);
output UDP_OUT;
input UDP_IN;
input VPWR;
assign UDP_OUT = UDP_IN;
endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2o (X,A1_N,A2_N,B1,B2);


    
    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    wire and0_out ;
    wire nor0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , B1, B2            );
    nor nor0 (nor0_out , A1_N, A2_N        );
    or  or0  (or0_out_X, nor0_out, and0_out);
    buf buf0 (X        , or0_out_X         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2o_1 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2o base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2o_2 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2o base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2o_4 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2o base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2oi (Y,A1_N,A2_N,B1,B2);


    
    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    wire and0_out  ;
    wire nor0_out  ;
    wire nor1_out_Y;

    
    and and0 (and0_out  , B1, B2            );
    nor nor0 (nor0_out  , A1_N, A2_N        );
    nor nor1 (nor1_out_Y, nor0_out, and0_out);
    buf buf0 (Y         , nor1_out_Y        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2oi_1 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2oi base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2oi_2 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2oi base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2bb2oi_4 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2bb2oi base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21bo (X,A1,A2,B1_N);


    
    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    wire nand0_out  ;
    wire nand1_out_X;

    
    nand nand0 (nand0_out  , A2, A1         );
    nand nand1 (nand1_out_X, B1_N, nand0_out);
    buf  buf0  (X          , nand1_out_X    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21bo_1 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21bo base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21bo_2 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21bo base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21bo_4 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21bo base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21boi (Y,A1,A2,B1_N);


    
    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    wire b         ;
    wire and0_out  ;
    wire nor0_out_Y;

    
    not not0 (b         , B1_N           );
    and and0 (and0_out  , A1, A2         );
    nor nor0 (nor0_out_Y, b, and0_out    );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21boi_0 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21boi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21boi_1 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21boi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21boi_2 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21boi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21boi_4 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21boi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21o (X,A1,A2,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A1, A2         );
    or  or0  (or0_out_X, and0_out, B1   );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21o_1 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21o_2 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21o_4 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21oi (Y,A1,A2,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A1, A2         );
    nor nor0 (nor0_out_Y, B1, and0_out   );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21oi_1 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21oi_2 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a21oi_4 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a21oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22o (X,A1,A2,B1,B2);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    wire and0_out ;
    wire and1_out ;
    wire or0_out_X;

    
    and and0 (and0_out , B1, B2            );
    and and1 (and1_out , A1, A2            );
    or  or0  (or0_out_X, and1_out, and0_out);
    buf buf0 (X        , or0_out_X         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22o_1 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22o_2 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22o_4 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22oi (Y,A1,A2,B1,B2);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    wire nand0_out ;
    wire nand1_out ;
    wire and0_out_Y;

    
    nand nand0 (nand0_out , A2, A1              );
    nand nand1 (nand1_out , B2, B1              );
    and  and0  (and0_out_Y, nand0_out, nand1_out);
    buf  buf0  (Y         , and0_out_Y          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22oi_1 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22oi_2 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a22oi_4 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a22oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31o (X,A1,A2,A3,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A3, A1, A2     );
    or  or0  (or0_out_X, and0_out, B1   );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31o_1 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31o_2 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31o_4 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31oi (Y,A1,A2,A3,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A3, A1, A2     );
    nor nor0 (nor0_out_Y, B1, and0_out   );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31oi_1 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31oi_2 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a31oi_4 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a31oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32o (X,A1,A2,A3,B1,B2);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    wire and0_out ;
    wire and1_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A3, A1, A2        );
    and and1 (and1_out , B1, B2            );
    or  or0  (or0_out_X, and1_out, and0_out);
    buf buf0 (X        , or0_out_X         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32o_1 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32o_2 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32o_4 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32oi (Y,A1,A2,A3,B1,B2);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    wire nand0_out ;
    wire nand1_out ;
    wire and0_out_Y;

    
    nand nand0 (nand0_out , A2, A1, A3          );
    nand nand1 (nand1_out , B2, B1              );
    and  and0  (and0_out_Y, nand0_out, nand1_out);
    buf  buf0  (Y         , and0_out_Y          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32oi_1 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32oi_2 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a32oi_4 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a32oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41o (X,A1,A2,A3,A4,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A1, A2, A3, A4 );
    or  or0  (or0_out_X, and0_out, B1   );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41o_1 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41o_2 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41o_4 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41oi (Y,A1,A2,A3,A4,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A1, A2, A3, A4 );
    nor nor0 (nor0_out_Y, B1, and0_out   );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41oi_1 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41oi_2 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a41oi_4 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a41oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211o (X,A1,A2,B1,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A1, A2          );
    or  or0  (or0_out_X, and0_out, C1, B1);
    buf buf0 (X        , or0_out_X       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211o_1 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211o_2 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211o_4 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211oi (Y,A1,A2,B1,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A1, A2          );
    nor nor0 (nor0_out_Y, and0_out, B1, C1);
    buf buf0 (Y         , nor0_out_Y      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211oi_1 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211oi_2 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a211oi_4 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a211oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221o (X,A1,A2,B1,B2,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    wire and0_out ;
    wire and1_out ;
    wire or0_out_X;

    
    and and0 (and0_out , B1, B2                );
    and and1 (and1_out , A1, A2                );
    or  or0  (or0_out_X, and1_out, and0_out, C1);
    buf buf0 (X        , or0_out_X             );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221o_1 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221o_2 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221o_4 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221oi (Y,A1,A2,B1,B2,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    wire and0_out  ;
    wire and1_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , B1, B2                );
    and and1 (and1_out  , A1, A2                );
    nor nor0 (nor0_out_Y, and0_out, C1, and1_out);
    buf buf0 (Y         , nor0_out_Y            );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221oi_1 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221oi_2 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a221oi_4 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a221oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a222oi (Y,A1,A2,B1,B2,C1,C2);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;
    input  C2;

    
    wire nand0_out ;
    wire nand1_out ;
    wire nand2_out ;
    wire and0_out_Y;

    
    nand nand0 (nand0_out , A2, A1                         );
    nand nand1 (nand1_out , B2, B1                         );
    nand nand2 (nand2_out , C2, C1                         );
    and  and0  (and0_out_Y, nand0_out, nand1_out, nand2_out);
    buf  buf0  (Y         , and0_out_Y                     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a222oi_1 (Y,A1,A2,B1,B2,C1,C2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;
    input  C2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a222oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1),
        .C2(C2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311o (X,A1,A2,A3,B1,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A3, A1, A2      );
    or  or0  (or0_out_X, and0_out, C1, B1);
    buf buf0 (X        , or0_out_X       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311o_1 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311o_2 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311o_4 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311oi (Y,A1,A2,A3,B1,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A3, A1, A2      );
    nor nor0 (nor0_out_Y, and0_out, B1, C1);
    buf buf0 (Y         , nor0_out_Y      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311oi_1 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311oi_2 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a311oi_4 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a311oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111o (X,A1,A2,B1,C1,D1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    wire and0_out ;
    wire or0_out_X;

    
    and and0 (and0_out , A1, A2              );
    or  or0  (or0_out_X, C1, B1, and0_out, D1);
    buf buf0 (X        , or0_out_X           );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111o_1 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111o_2 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111o_4 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111o base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111oi (Y,A1,A2,B1,C1,D1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    wire and0_out  ;
    wire nor0_out_Y;

    
    and and0 (and0_out  , A1, A2              );
    nor nor0 (nor0_out_Y, B1, C1, D1, and0_out);
    buf buf0 (Y         , nor0_out_Y          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111oi_0 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111oi_1 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111oi_2 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__a2111oi_4 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__a2111oi base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2 (X,A,B);


    
    output X;
    input  A;
    input  B;

    
    wire and0_out_X;

    
    and and0 (and0_out_X, A, B           );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2_0 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2_1 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2_2 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2_4 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2b (X,A_N,B);


    
    output X  ;
    input  A_N;
    input  B  ;

    
    wire not0_out  ;
    wire and0_out_X;

    
    not not0 (not0_out  , A_N            );
    and and0 (and0_out_X, not0_out, B    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2b_1 (X,A_N,B);


    output X  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2b base (
        .X(X),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2b_2 (X,A_N,B);


    output X  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2b base (
        .X(X),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and2b_4 (X,A_N,B);


    output X  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and2b base (
        .X(X),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3 (X,A,B,C);


    
    output X;
    input  A;
    input  B;
    input  C;

    
    wire and0_out_X;

    
    and and0 (and0_out_X, C, A, B        );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3_1 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3_2 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3_4 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3b (X,A_N,B,C);


    
    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    wire not0_out  ;
    wire and0_out_X;

    
    not not0 (not0_out  , A_N            );
    and and0 (and0_out_X, C, not0_out, B );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3b_1 (X,A_N,B,C);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3b_2 (X,A_N,B,C);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and3b_4 (X,A_N,B,C);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and3b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4 (X,A,B,C,D);


    
    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    wire and0_out_X;

    
    and and0 (and0_out_X, A, B, C, D     );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4_1 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4_2 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4_4 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4b (X,A_N,B,C,D);


    
    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    wire not0_out  ;
    wire and0_out_X;

    
    not not0 (not0_out  , A_N              );
    and and0 (and0_out_X, not0_out, B, C, D);
    buf buf0 (X         , and0_out_X       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4b_1 (X,A_N,B,C,D);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4b_2 (X,A_N,B,C,D);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4b_4 (X,A_N,B,C,D);


    output X  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4b base (
        .X(X),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4bb (X,A_N,B_N,C,D);


    
    output X  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    wire nor0_out  ;
    wire and0_out_X;

    
    nor nor0 (nor0_out  , A_N, B_N       );
    and and0 (and0_out_X, nor0_out, C, D );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4bb_1 (X,A_N,B_N,C,D);


    output X  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4bb base (
        .X(X),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4bb_2 (X,A_N,B_N,C,D);


    output X  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4bb base (
        .X(X),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__and4bb_4 (X,A_N,B_N,C,D);


    output X  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__and4bb base (
        .X(X),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_4 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_6 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_8 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_12 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__buf_16 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__buf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufbuf (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufbuf_8 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__bufbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufbuf_16 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__bufbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufinv (Y,A);


    
    output Y;
    input  A;

    
    wire not0_out_Y;

    
    not not0 (not0_out_Y, A              );
    buf buf0 (Y         , not0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufinv_8 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__bufinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__bufinv_16 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__bufinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf_4 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf_8 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkbuf_16 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkbuf base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s15 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s15_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s15 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s15_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s15 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s18 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s18_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s18 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s18_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s18 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s25 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s25_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s25 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s25_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s25 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s50 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s50_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s50 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkdlybuf4s50_2 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkdlybuf4s50 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv (Y,A);


    
    output Y;
    input  A;

    
    wire not0_out_Y;

    
    not not0 (not0_out_Y, A              );
    buf buf0 (Y         , not0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv_1 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv_2 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv_4 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv_8 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinv_16 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinvlp (Y,A);


    
    output Y;
    input  A;

    
    wire not0_out_Y;

    
    not not0 (not0_out_Y, A              );
    buf buf0 (Y         , not0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinvlp_2 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinvlp base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__clkinvlp_4 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__clkinvlp base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__conb (HI,LO);


    
    output HI;
    output LO;

    
    mod_pullup   pullup0   (HI    );
    mod_pulldown pulldown0 (LO    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__conb_1 (HI,LO);


    output HI;
    output LO;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__conb base (
        .HI(HI),
        .LO(LO)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap_3 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap_4 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap_6 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap_8 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__decap_12 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__decap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfbbn (Q,Q_N,D,CLK_N,SET_B,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  D      ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    wire RESET;
    wire SET  ;
    wire CLK  ;
    wire buf_Q;

    
    not                                      not0 (RESET , RESET_B           );
    not                                      not1 (SET   , SET_B             );
    not                                      not2 (CLK   , CLK_N             );
    sky130_fd_sc_hd__udp_dff$NSR  dff0 (buf_Q , SET, RESET, CLK, D);
    buf                                      buf0 (Q     , buf_Q             );
    not                                      not3 (Q_N   , buf_Q             );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfbbn_1 (Q,Q_N,D,CLK_N,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfbbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .CLK_N(CLK_N),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfbbn_2 (Q,Q_N,D,CLK_N,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfbbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .CLK_N(CLK_N),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfbbp (Q,Q_N,D,CLK,SET_B,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  D      ;
    input  CLK    ;
    input  SET_B  ;
    input  RESET_B;

    
    wire RESET;
    wire SET  ;
    wire buf_Q;

    
    not                                      not0 (RESET , RESET_B           );
    not                                      not1 (SET   , SET_B             );
    sky130_fd_sc_hd__udp_dff$NSR  dff0 (buf_Q , SET, RESET, CLK, D);
    buf                                      buf0 (Q     , buf_Q             );
    not                                      not2 (Q_N   , buf_Q             );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfbbp_1 (Q,Q_N,D,CLK,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  CLK    ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfbbp base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .CLK(CLK),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrbp (Q,Q_N,CLK,D,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    wire buf_Q;
    wire RESET;

    
    not                                     not0 (RESET , RESET_B        );
    sky130_fd_sc_hd__udp_dff$PR  dff0 (buf_Q , D, CLK, RESET  );
    buf                                     buf0 (Q     , buf_Q          );
    not                                     not1 (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrbp_1 (Q,Q_N,CLK,D,RESET_B);


    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrbp_2 (Q,Q_N,CLK,D,RESET_B);


    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtn (Q,CLK_N,D,RESET_B);


    
    output Q      ;
    input  CLK_N  ;
    input  D      ;
    input  RESET_B;

    
    wire buf_Q ;
    wire RESET ;
    wire intclk;

    
    not                                     not0 (RESET , RESET_B         );
    not                                     not1 (intclk, CLK_N           );
    sky130_fd_sc_hd__udp_dff$PR  dff0 (buf_Q , D, intclk, RESET);
    buf                                     buf0 (Q     , buf_Q           );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtn_1 (Q,CLK_N,D,RESET_B);


    output Q      ;
    input  CLK_N  ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrtn base (
        .Q(Q),
        .CLK_N(CLK_N),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtp (Q,CLK,D,RESET_B);


    
    output Q      ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    wire buf_Q;
    wire RESET;

    
    not                                     not0 (RESET , RESET_B        );
    sky130_fd_sc_hd__udp_dff$PR  dff0 (buf_Q , D, CLK, RESET  );
    buf                                     buf0 (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtp_1 (Q,CLK,D,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtp_2 (Q,CLK,D,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfrtp_4 (Q,CLK,D,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfsbp (Q,Q_N,CLK,D,SET_B);


    
    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    wire buf_Q;
    wire SET  ;

    
    not                                     not0 (SET   , SET_B          );
    sky130_fd_sc_hd__udp_dff$PS  dff0 (buf_Q , D, CLK, SET    );
    buf                                     buf0 (Q     , buf_Q          );
    not                                     not1 (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfsbp_1 (Q,Q_N,CLK,D,SET_B);


    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfsbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfsbp_2 (Q,Q_N,CLK,D,SET_B);


    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfsbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfstp (Q,CLK,D,SET_B);


    
    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    wire buf_Q;
    wire SET  ;

    
    not                                     not0 (SET   , SET_B          );
    sky130_fd_sc_hd__udp_dff$PS  dff0 (buf_Q , D, CLK, SET    );
    buf                                     buf0 (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfstp_1 (Q,CLK,D,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfstp_2 (Q,CLK,D,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfstp_4 (Q,CLK,D,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxbp (Q,Q_N,CLK,D);


    
    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;

    
    wire buf_Q;

    
    sky130_fd_sc_hd__udp_dff$P  dff0 (buf_Q , D, CLK         );
    buf                                    buf0 (Q     , buf_Q          );
    not                                    not0 (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxbp_1 (Q,Q_N,CLK,D);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxbp_2 (Q,Q_N,CLK,D);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxtp (Q,CLK,D);


    
    output Q  ;
    input  CLK;
    input  D  ;

    
    wire buf_Q;

    
    sky130_fd_sc_hd__udp_dff$P  dff0 (buf_Q , D, CLK         );
    buf                                    buf0 (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxtp_1 (Q,CLK,D);


    output Q  ;
    input  CLK;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxtp_2 (Q,CLK,D);


    output Q  ;
    input  CLK;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dfxtp_4 (Q,CLK,D);


    output Q  ;
    input  CLK;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__diode (DIODE);


    
    input DIODE;
     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__diode_2 (DIODE);


    input DIODE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__diode base (
        .DIODE(DIODE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlclkp (GCLK,GATE,CLK);


    
    output GCLK;
    input  GATE;
    input  CLK ;

    
    wire m0  ;
    wire clkn;

    
    not                           not0    (clkn  , CLK            );
    sky130_fd_sc_hd__udp_dlatch$P dlatch0 (m0    , GATE, clkn     );
    and                           and0    (GCLK  , m0, CLK        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlclkp_1 (GCLK,GATE,CLK);


    output GCLK;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlclkp base (
        .GCLK(GCLK),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlclkp_2 (GCLK,GATE,CLK);


    output GCLK;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlclkp base (
        .GCLK(GCLK),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlclkp_4 (GCLK,GATE,CLK);


    output GCLK;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlclkp base (
        .GCLK(GCLK),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbn (Q,Q_N,RESET_B,D,GATE_N);


    
    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    wire RESET  ;
    wire intgate;
    wire buf_Q  ;

    
    not                                        not0    (RESET  , RESET_B          );
    not                                        not1    (intgate, GATE_N           );
    sky130_fd_sc_hd__udp_dlatch$PR  dlatch0 (buf_Q  , D, intgate, RESET);
    buf                                        buf0    (Q      , buf_Q            );
    not                                        not2    (Q_N    , buf_Q            );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbn_1 (Q,Q_N,RESET_B,D,GATE_N);


    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrbn base (
        .Q(Q),
        .Q_N(Q_N),
        .RESET_B(RESET_B),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbn_2 (Q,Q_N,RESET_B,D,GATE_N);


    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrbn base (
        .Q(Q),
        .Q_N(Q_N),
        .RESET_B(RESET_B),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbp (Q,Q_N,RESET_B,D,GATE);


    
    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    wire RESET;
    wire buf_Q;

    
    not                                        not0    (RESET , RESET_B        );
    sky130_fd_sc_hd__udp_dlatch$PR  dlatch0 (buf_Q , D, GATE, RESET );
    buf                                        buf0    (Q     , buf_Q          );
    not                                        not1    (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbp_1 (Q,Q_N,RESET_B,D,GATE);


    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .RESET_B(RESET_B),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrbp_2 (Q,Q_N,RESET_B,D,GATE);


    output Q      ;
    output Q_N    ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .RESET_B(RESET_B),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtn (Q,RESET_B,D,GATE_N);


    
    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    wire RESET  ;
    wire intgate;
    wire buf_Q  ;

    
    not                                        not0    (RESET  , RESET_B          );
    not                                        not1    (intgate, GATE_N           );
    sky130_fd_sc_hd__udp_dlatch$PR  dlatch0 (buf_Q  , D, intgate, RESET);
    buf                                        buf0    (Q      , buf_Q            );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtn_1 (Q,RESET_B,D,GATE_N);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtn base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtn_2 (Q,RESET_B,D,GATE_N);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtn base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtn_4 (Q,RESET_B,D,GATE_N);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE_N ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtn base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtp (Q,RESET_B,D,GATE);


    
    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    wire RESET;
    wire buf_Q;

    
    not                                        not0    (RESET , RESET_B        );
    sky130_fd_sc_hd__udp_dlatch$PR  dlatch0 (buf_Q , D, GATE, RESET );
    buf                                        buf0    (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtp_1 (Q,RESET_B,D,GATE);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtp base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtp_2 (Q,RESET_B,D,GATE);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtp base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlrtp_4 (Q,RESET_B,D,GATE);


    output Q      ;
    input  RESET_B;
    input  D      ;
    input  GATE   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlrtp base (
        .Q(Q),
        .RESET_B(RESET_B),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxbn (Q,Q_N,D,GATE_N);


    
    output Q     ;
    output Q_N   ;
    input  D     ;
    input  GATE_N;

    
    wire GATE ;
    wire buf_Q;

    
    not                                       not0    (GATE  , GATE_N         );
    sky130_fd_sc_hd__udp_dlatch$P  dlatch0 (buf_Q , D, GATE        );
    buf                                       buf0    (Q     , buf_Q          );
    not                                       not1    (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxbn_1 (Q,Q_N,D,GATE_N);


    output Q     ;
    output Q_N   ;
    input  D     ;
    input  GATE_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxbn_2 (Q,Q_N,D,GATE_N);


    output Q     ;
    output Q_N   ;
    input  D     ;
    input  GATE_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxbp (Q,Q_N,D,GATE);


    
    output Q   ;
    output Q_N ;
    input  D   ;
    input  GATE;

    
    wire buf_Q;

    
    sky130_fd_sc_hd__udp_dlatch$P  dlatch0 (buf_Q , D, GATE        );
    buf                                       buf0    (Q     , buf_Q          );
    not                                       not0    (Q_N   , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxbp_1 (Q,Q_N,D,GATE);


    output Q   ;
    output Q_N ;
    input  D   ;
    input  GATE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtn (Q,D,GATE_N);


    
    output Q     ;
    input  D     ;
    input  GATE_N;

    
    wire GATE ;
    wire buf_Q;

    
    not                           not0    (GATE  , GATE_N         );
    sky130_fd_sc_hd__udp_dlatch$P dlatch0 (buf_Q , D, GATE        );
    buf                           buf0    (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtn_1 (Q,D,GATE_N);


    output Q     ;
    input  D     ;
    input  GATE_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxtn base (
        .Q(Q),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtn_2 (Q,D,GATE_N);


    output Q     ;
    input  D     ;
    input  GATE_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxtn base (
        .Q(Q),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtn_4 (Q,D,GATE_N);


    output Q     ;
    input  D     ;
    input  GATE_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxtn base (
        .Q(Q),
        .D(D),
        .GATE_N(GATE_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtp (Q,D,GATE);


    
    output Q   ;
    input  D   ;
    input  GATE;

    
    wire buf_Q;

    
    sky130_fd_sc_hd__udp_dlatch$P dlatch0 (buf_Q , D, GATE        );
    buf                           buf0    (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlxtp_1 (Q,D,GATE);


    output Q   ;
    input  D   ;
    input  GATE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlxtp base (
        .Q(Q),
        .D(D),
        .GATE(GATE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd1 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd1_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlygate4sd1 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd2 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd2_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlygate4sd2 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd3 (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlygate4sd3_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlygate4sd3 base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s2s (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s2s_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlymetal6s2s base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s4s (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s4s_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlymetal6s4s base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s6s (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__dlymetal6s6s_1 (X,A);


    output X;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__dlymetal6s6s base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ebufn (Z,A,TE_B);


    
    output Z   ;
    input  A   ;
    input  TE_B;

    
    bufif0 bufif00 (Z     , A, TE_B        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ebufn_1 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ebufn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ebufn_2 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ebufn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ebufn_4 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ebufn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ebufn_8 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ebufn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__edfxbp (Q,Q_N,CLK,D,DE);


    
    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  DE ;

    
    wire buf_Q  ;
    wire mux_out;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, buf_Q, D, DE   );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );
    not                                       not0      (Q_N    , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__edfxbp_1 (Q,Q_N,CLK,D,DE);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  DE ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__edfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .DE(DE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__edfxtp (Q,CLK,D,DE);


    
    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;

    
    wire buf_Q  ;
    wire mux_out;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, buf_Q, D, DE   );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__edfxtp_1 (Q,CLK,D,DE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__edfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .DE(DE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn (Z,A,TE_B);


    
    output Z   ;
    input  A   ;
    input  TE_B;

    
    notif0 notif00 (Z     , A, TE_B        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn_0 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn_1 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn_2 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn_4 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvn_8 (Z,A,TE_B);


    output Z   ;
    input  A   ;
    input  TE_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvn base (
        .Z(Z),
        .A(A),
        .TE_B(TE_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvp (Z,A,TE);


    
    output Z ;
    input  A ;
    input  TE;

    
    notif1 notif10 (Z     , A, TE          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvp_1 (Z,A,TE);


    output Z ;
    input  A ;
    input  TE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvp base (
        .Z(Z),
        .A(A),
        .TE(TE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvp_2 (Z,A,TE);


    output Z ;
    input  A ;
    input  TE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvp base (
        .Z(Z),
        .A(A),
        .TE(TE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvp_4 (Z,A,TE);


    output Z ;
    input  A ;
    input  TE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvp base (
        .Z(Z),
        .A(A),
        .TE(TE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__einvp_8 (Z,A,TE);


    output Z ;
    input  A ;
    input  TE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__einvp base (
        .Z(Z),
        .A(A),
        .TE(TE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fa (COUT,SUM,A,B,CIN);


    
    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    wire or0_out     ;
    wire and0_out    ;
    wire and1_out    ;
    wire and2_out    ;
    wire nor0_out    ;
    wire nor1_out    ;
    wire or1_out_COUT;
    wire or2_out_SUM ;

    
    or  or0  (or0_out     , CIN, B            );
    and and0 (and0_out    , or0_out, A        );
    and and1 (and1_out    , B, CIN            );
    or  or1  (or1_out_COUT, and1_out, and0_out);
    buf buf0 (COUT        , or1_out_COUT      );
    and and2 (and2_out    , CIN, A, B         );
    nor nor0 (nor0_out    , A, or0_out        );
    nor nor1 (nor1_out    , nor0_out, COUT    );
    or  or2  (or2_out_SUM , nor1_out, and2_out);
    buf buf1 (SUM         , or2_out_SUM       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fa_1 (COUT,SUM,A,B,CIN);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fa base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CIN(CIN)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fa_2 (COUT,SUM,A,B,CIN);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fa base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CIN(CIN)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fa_4 (COUT,SUM,A,B,CIN);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fa base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CIN(CIN)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fah (COUT,SUM,A,B,CI);


    
    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CI  ;

    
    wire xor0_out_SUM;
    wire a_b         ;
    wire a_ci        ;
    wire b_ci        ;
    wire or0_out_COUT;

    
    xor xor0 (xor0_out_SUM, A, B, CI       );
    buf buf0 (SUM         , xor0_out_SUM   );
    and and0 (a_b         , A, B           );
    and and1 (a_ci        , A, CI          );
    and and2 (b_ci        , B, CI          );
    or  or0  (or0_out_COUT, a_b, a_ci, b_ci);
    buf buf1 (COUT        , or0_out_COUT   );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fah_1 (COUT,SUM,A,B,CI);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CI  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fah base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CI(CI)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fahcin (COUT,SUM,A,B,CIN);


    
    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    wire ci          ;
    wire xor0_out_SUM;
    wire a_b         ;
    wire a_ci        ;
    wire b_ci        ;
    wire or0_out_COUT;

    
    not not0 (ci          , CIN            );
    xor xor0 (xor0_out_SUM, A, B, ci       );
    buf buf0 (SUM         , xor0_out_SUM   );
    and and0 (a_b         , A, B           );
    and and1 (a_ci        , A, ci          );
    and and2 (b_ci        , B, ci          );
    or  or0  (or0_out_COUT, a_b, a_ci, b_ci);
    buf buf1 (COUT        , or0_out_COUT   );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fahcin_1 (COUT,SUM,A,B,CIN);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;
    input  CIN ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fahcin base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CIN(CIN)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fahcon (COUT_N,SUM,A,B,CI);


    
    output COUT_N;
    output SUM   ;
    input  A     ;
    input  B     ;
    input  CI    ;

    
    wire xor0_out_SUM ;
    wire a_b          ;
    wire a_ci         ;
    wire b_ci         ;
    wire or0_out_coutn;

    
    xor xor0 (xor0_out_SUM , A, B, CI       );
    buf buf0 (SUM          , xor0_out_SUM   );
    nor nor0 (a_b          , A, B           );
    nor nor1 (a_ci         , A, CI          );
    nor nor2 (b_ci         , B, CI          );
    or  or0  (or0_out_coutn, a_b, a_ci, b_ci);
    buf buf1 (COUT_N       , or0_out_coutn  );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fahcon_1 (COUT_N,SUM,A,B,CI);


    output COUT_N;
    output SUM   ;
    input  A     ;
    input  B     ;
    input  CI    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fahcon base (
        .COUT_N(COUT_N),
        .SUM(SUM),
        .A(A),
        .B(B),
        .CI(CI)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fill ();


    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;
     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__fill_1 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fill base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fill_2 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fill base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fill_4 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fill base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__fill_8 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__fill base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ha (COUT,SUM,A,B);


    
    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;

    
    wire and0_out_COUT;
    wire xor0_out_SUM ;

    
    and and0 (and0_out_COUT, A, B           );
    buf buf0 (COUT         , and0_out_COUT  );
    xor xor0 (xor0_out_SUM , B, A           );
    buf buf1 (SUM          , xor0_out_SUM   );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ha_1 (COUT,SUM,A,B);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ha base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ha_2 (COUT,SUM,A,B);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ha base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__ha_4 (COUT,SUM,A,B);


    output COUT;
    output SUM ;
    input  A   ;
    input  B   ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__ha base (
        .COUT(COUT),
        .SUM(SUM),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv (Y,A);


    
    output Y;
    input  A;

    
    wire not0_out_Y;

    
    not not0 (not0_out_Y, A              );
    buf buf0 (Y         , not0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_1 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_2 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_4 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_6 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_8 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_12 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__inv_16 (Y,A);


    output Y;
    input  A;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__inv base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_bleeder (SHORT);


    input SHORT;


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_bleeder_1 (SHORT);


    input SHORT;

    
    wire    VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_bleeder base (
        .SHORT(SHORT)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr_1 (X,A);


    output X;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkbufkapwr base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr_2 (X,A);


    output X;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkbufkapwr base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr_4 (X,A);


    output X;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkbufkapwr base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr_8 (X,A);


    output X;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkbufkapwr base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkbufkapwr_16 (X,A);


    output X;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkbufkapwr base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr (Y,A);


    
    output Y;
    input  A;

    
    wire not0_out_Y;

    
    not not0 (not0_out_Y, A              );
    buf buf0 (Y         , not0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr_1 (Y,A);


    output Y;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkinvkapwr base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr_2 (Y,A);


    output Y;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkinvkapwr base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr_4 (Y,A);


    output Y;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkinvkapwr base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr_8 (Y,A);


    output Y;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkinvkapwr base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_clkinvkapwr_16 (Y,A);


    output Y;
    input  A;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_clkinvkapwr base (
        .Y(Y),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr_3 ();

    
    supply1 VPWR ;
    supply1 KAPWR;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_decapkapwr base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr_4 ();

    
    supply1 VPWR ;
    supply1 KAPWR;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_decapkapwr base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr_6 ();

    
    supply1 VPWR ;
    supply1 KAPWR;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_decapkapwr base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr_8 ();

    
    supply1 VPWR ;
    supply1 KAPWR;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_decapkapwr base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_decapkapwr_12 ();

    
    supply1 VPWR ;
    supply1 KAPWR;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_decapkapwr base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso0n (X,A,SLEEP_B);


    
    output X      ;
    input  A      ;
    input  SLEEP_B;

    
    and and0 (X     , A, SLEEP_B     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso0n_1 (X,A,SLEEP_B);


    output X      ;
    input  A      ;
    input  SLEEP_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_inputiso0n base (
        .X(X),
        .A(A),
        .SLEEP_B(SLEEP_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso0p (X,A,SLEEP);


    
    output X    ;
    input  A    ;
    input  SLEEP;

    
    wire sleepn;

    
    not not0 (sleepn, SLEEP          );
    and and0 (X     , A, sleepn      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso0p_1 (X,A,SLEEP);


    output X    ;
    input  A    ;
    input  SLEEP;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_inputiso0p base (
        .X(X),
        .A(A),
        .SLEEP(SLEEP)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso1n (X,A,SLEEP_B);


    
    output X      ;
    input  A      ;
    input  SLEEP_B;

    
    wire SLEEP;

    
    not not0 (SLEEP , SLEEP_B        );
    or  or0  (X     , A, SLEEP       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso1n_1 (X,A,SLEEP_B);


    output X      ;
    input  A      ;
    input  SLEEP_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_inputiso1n base (
        .X(X),
        .A(A),
        .SLEEP_B(SLEEP_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso1p (X,A,SLEEP);


    
    output X    ;
    input  A    ;
    input  SLEEP;

    
    or  or0  (X     , A, SLEEP       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputiso1p_1 (X,A,SLEEP);


    output X    ;
    input  A    ;
    input  SLEEP;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_inputiso1p base (
        .X(X),
        .A(A),
        .SLEEP(SLEEP)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputisolatch (Q,D,SLEEP_B);


    
    output Q      ;
    input  D      ;
    input  SLEEP_B;

    
    wire buf_Q;

    
    sky130_fd_sc_hd__udp_dlatch$lP dlatch0 (buf_Q , D, SLEEP_B     );
    buf                            buf0    (Q     , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_inputisolatch_1 (Q,D,SLEEP_B);


    output Q      ;
    input  D      ;
    input  SLEEP_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_inputisolatch base (
        .Q(Q),
        .D(D),
        .SLEEP_B(SLEEP_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc (X,SLEEP,A);


    
    output X    ;
    input  SLEEP;
    input  A    ;

    
    wire not0_out  ;
    wire and0_out_X;

    
    not not0 (not0_out  , SLEEP          );
    and and0 (and0_out_X, not0_out, A    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc_1 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_isobufsrc base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc_2 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_isobufsrc base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc_4 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_isobufsrc base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc_8 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_isobufsrc base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrc_16 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__lpflow_isobufsrc base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrckapwr (X,SLEEP,A);


    
    output X    ;
    input  SLEEP;
    input  A    ;

    
    wire not0_out  ;
    wire and0_out_X;

    
    not not0 (not0_out  , SLEEP          );
    and and0 (and0_out_X, not0_out, A    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_isobufsrckapwr_16 (X,SLEEP,A);


    output X    ;
    input  SLEEP;
    input  A    ;

    
    supply1 KAPWR;
    supply1 VPWR ;
    supply0 VGND ;
    supply1 VPB  ;
    supply0 VNB  ;

    sky130_fd_sc_hd__lpflow_isobufsrckapwr base (
        .X(X),
        .SLEEP(SLEEP),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap (X,A);


    
    output X;
    input  A;

    
    buf buf0 (X     , A              );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap_1 (X,A);


    output X;
    input  A;

    
    wire    VPWRIN;
    supply1 VPWR  ;
    supply0 VGND  ;
    supply1 VPB   ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap_2 (X,A);


    output X;
    input  A;

    
    wire    VPWRIN;
    supply1 VPWR  ;
    supply0 VGND  ;
    supply1 VPB   ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap_4 (X,A);


    output X;
    input  A;

    
    wire    VPWRIN;
    supply1 VPWR  ;
    supply0 VGND  ;
    supply1 VPB   ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_hl_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell (X,A);


    
    output X;
    input  A;

    
    buf buf0 (X     , A              );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_4 (X,A);


    output X;
    input  A;

    
    wire    LOWLVPWR;
    supply1 VPWR    ;
    supply0 VGND    ;
    supply1 VPB     ;
    supply0 VNB     ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap (X,A);


    
    output X;
    input  A;

    
    buf buf0 (X     , A              );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap_1 (X,A);


    output X;
    input  A;

    
    wire    LOWLVPWR;
    supply1 VPWR    ;
    supply0 VGND    ;
    supply1 VPB     ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap_2 (X,A);


    output X;
    input  A;

    
    wire    LOWLVPWR;
    supply1 VPWR    ;
    supply0 VGND    ;
    supply1 VPB     ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap_4 (X,A);


    output X;
    input  A;

    
    wire    LOWLVPWR;
    supply1 VPWR    ;
    supply0 VGND    ;
    supply1 VPB     ;

    sky130_fd_sc_hd__lpflow_lsbuf_lh_isowell_tap base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__macro_sparecell (LO);


    
    output LO;

    
    wire nor2left ;
    wire invleft  ;
    wire nor2right;
    wire invright ;
    wire nd2left  ;
    wire nd2right ;
    wire tielo    ;
    wire net7     ;

    
    sky130_fd_sc_hd__inv_2   inv0   (.A(nor2left) , .Y(invleft)                );
    sky130_fd_sc_hd__inv_2   inv1   (.A(nor2right), .Y(invright)               );
    sky130_fd_sc_hd__nor2_2  nor20  (.B(nd2left)  , .A(nd2left), .Y(nor2left)  );
    sky130_fd_sc_hd__nor2_2  nor21  (.B(nd2right) , .A(nd2right), .Y(nor2right));
    sky130_fd_sc_hd__nand2_2 nand20 (.B(tielo)    , .A(tielo), .Y(nd2right)    );
    sky130_fd_sc_hd__nand2_2 nand21 (.B(tielo)    , .A(tielo), .Y(nd2left)     );
    sky130_fd_sc_hd__conb_1  conb0  (.LO(tielo)   , .HI(net7)                  );
    buf                      buf0   (LO           , tielo                      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__maj3 (X,A,B,C);


    
    output X;
    input  A;
    input  B;
    input  C;

    
    wire or0_out  ;
    wire and0_out ;
    wire and1_out ;
    wire or1_out_X;

    
    or  or0  (or0_out  , B, A              );
    and and0 (and0_out , or0_out, C        );
    and and1 (and1_out , A, B              );
    or  or1  (or1_out_X, and1_out, and0_out);
    buf buf0 (X        , or1_out_X         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__maj3_1 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__maj3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__maj3_2 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__maj3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__maj3_4 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__maj3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2 (X,A0,A1,S);


    
    output X ;
    input  A0;
    input  A1;
    input  S ;

    
    wire mux_2to10_out_X;

    
    sky130_fd_sc_hd__udp_mux_2to1 mux_2to10 (mux_2to10_out_X, A0, A1, S      );
    buf                           buf0      (X              , mux_2to10_out_X);


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2_1 (X,A0,A1,S);


    output X ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2_2 (X,A0,A1,S);


    output X ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2_4 (X,A0,A1,S);


    output X ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2_8 (X,A0,A1,S);


    output X ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2i (Y,A0,A1,S);


    
    output Y ;
    input  A0;
    input  A1;
    input  S ;

    
    wire mux_2to1_n0_out_Y;

    
    sky130_fd_sc_hd__udp_mux_2to1_N mux_2to1_n0 (mux_2to1_n0_out_Y, A0, A1, S        );
    buf                             buf0        (Y                , mux_2to1_n0_out_Y);


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2i_1 (Y,A0,A1,S);


    output Y ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2i base (
        .Y(Y),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2i_2 (Y,A0,A1,S);


    output Y ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2i base (
        .Y(Y),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux2i_4 (Y,A0,A1,S);


    output Y ;
    input  A0;
    input  A1;
    input  S ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux2i base (
        .Y(Y),
        .A0(A0),
        .A1(A1),
        .S(S)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux4 (X,A0,A1,A2,A3,S0,S1);


    
    output X ;
    input  A0;
    input  A1;
    input  A2;
    input  A3;
    input  S0;
    input  S1;

    
    wire mux_4to20_out_X;

    
    sky130_fd_sc_hd__udp_mux_4to2 mux_4to20 (mux_4to20_out_X, A0, A1, A2, A3, S0, S1);
    buf                           buf0      (X              , mux_4to20_out_X       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux4_1 (X,A0,A1,A2,A3,S0,S1);


    output X ;
    input  A0;
    input  A1;
    input  A2;
    input  A3;
    input  S0;
    input  S1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux4 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .S0(S0),
        .S1(S1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux4_2 (X,A0,A1,A2,A3,S0,S1);


    output X ;
    input  A0;
    input  A1;
    input  A2;
    input  A3;
    input  S0;
    input  S1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux4 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .S0(S0),
        .S1(S1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__mux4_4 (X,A0,A1,A2,A3,S0,S1);


    output X ;
    input  A0;
    input  A1;
    input  A2;
    input  A3;
    input  S0;
    input  S1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__mux4 base (
        .X(X),
        .A0(A0),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .S0(S0),
        .S1(S1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2 (Y,A,B);


    
    output Y;
    input  A;
    input  B;

    
    wire nand0_out_Y;

    
    nand nand0 (nand0_out_Y, B, A           );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2_1 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2_2 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2_4 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2_8 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2b (Y,A_N,B);


    
    output Y  ;
    input  A_N;
    input  B  ;

    
    wire not0_out ;
    wire or0_out_Y;

    
    not not0 (not0_out , B              );
    or  or0  (or0_out_Y, not0_out, A_N  );
    buf buf0 (Y        , or0_out_Y      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2b_1 (Y,A_N,B);


    output Y  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2b base (
        .Y(Y),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2b_2 (Y,A_N,B);


    output Y  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2b base (
        .Y(Y),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand2b_4 (Y,A_N,B);


    output Y  ;
    input  A_N;
    input  B  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand2b base (
        .Y(Y),
        .A_N(A_N),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3 (Y,A,B,C);


    
    output Y;
    input  A;
    input  B;
    input  C;

    
    wire nand0_out_Y;

    
    nand nand0 (nand0_out_Y, B, A, C        );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3_1 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3_2 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3_4 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3b (Y,A_N,B,C);


    
    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    wire not0_out   ;
    wire nand0_out_Y;

    
    not  not0  (not0_out   , A_N            );
    nand nand0 (nand0_out_Y, B, not0_out, C );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3b_1 (Y,A_N,B,C);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3b_2 (Y,A_N,B,C);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand3b_4 (Y,A_N,B,C);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand3b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4 (Y,A,B,C,D);


    
    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    wire nand0_out_Y;

    
    nand nand0 (nand0_out_Y, D, C, B, A     );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4_1 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4_2 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4_4 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4b (Y,A_N,B,C,D);


    
    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    wire not0_out   ;
    wire nand0_out_Y;

    
    not  not0  (not0_out   , A_N              );
    nand nand0 (nand0_out_Y, D, C, B, not0_out);
    buf  buf0  (Y          , nand0_out_Y      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4b_1 (Y,A_N,B,C,D);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4b_2 (Y,A_N,B,C,D);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4b_4 (Y,A_N,B,C,D);


    output Y  ;
    input  A_N;
    input  B  ;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4b base (
        .Y(Y),
        .A_N(A_N),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4bb (Y,A_N,B_N,C,D);


    
    output Y  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    wire nand0_out;
    wire or0_out_Y;

    
    nand nand0 (nand0_out, D, C               );
    or   or0   (or0_out_Y, B_N, A_N, nand0_out);
    buf  buf0  (Y        , or0_out_Y          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4bb_1 (Y,A_N,B_N,C,D);


    output Y  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4bb base (
        .Y(Y),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4bb_2 (Y,A_N,B_N,C,D);


    output Y  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4bb base (
        .Y(Y),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nand4bb_4 (Y,A_N,B_N,C,D);


    output Y  ;
    input  A_N;
    input  B_N;
    input  C  ;
    input  D  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nand4bb base (
        .Y(Y),
        .A_N(A_N),
        .B_N(B_N),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2 (Y,A,B);


    
    output Y;
    input  A;
    input  B;

    
    wire nor0_out_Y;

    
    nor nor0 (nor0_out_Y, A, B           );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2_1 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2_2 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2_4 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2_8 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2b (Y,A,B_N);


    
    output Y  ;
    input  A  ;
    input  B_N;

    
    wire not0_out  ;
    wire and0_out_Y;

    
    not not0 (not0_out  , A              );
    and and0 (and0_out_Y, not0_out, B_N  );
    buf buf0 (Y         , and0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2b_1 (Y,A,B_N);


    output Y  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2b base (
        .Y(Y),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2b_2 (Y,A,B_N);


    output Y  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2b base (
        .Y(Y),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor2b_4 (Y,A,B_N);


    output Y  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor2b base (
        .Y(Y),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3 (Y,A,B,C);


    
    output Y;
    input  A;
    input  B;
    input  C;

    
    wire nor0_out_Y;

    
    nor nor0 (nor0_out_Y, C, A, B        );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3_1 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3_2 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3_4 (Y,A,B,C);


    output Y;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3b (Y,A,B,C_N);


    
    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    wire nor0_out  ;
    wire and0_out_Y;

    
    nor nor0 (nor0_out  , A, B           );
    and and0 (and0_out_Y, C_N, nor0_out  );
    buf buf0 (Y         , and0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3b_1 (Y,A,B,C_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3b_2 (Y,A,B,C_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor3b_4 (Y,A,B,C_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor3b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4 (Y,A,B,C,D);


    
    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    wire nor0_out_Y;

    
    nor nor0 (nor0_out_Y, A, B, C, D     );
    buf buf0 (Y         , nor0_out_Y     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4_1 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4_2 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4_4 (Y,A,B,C,D);


    output Y;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4 base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4b (Y,A,B,C,D_N);


    
    output Y  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    wire not0_out  ;
    wire nor0_out_Y;

    
    not not0 (not0_out  , D_N              );
    nor nor0 (nor0_out_Y, A, B, C, not0_out);
    buf buf0 (Y         , nor0_out_Y       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4b_1 (Y,A,B,C,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4b_2 (Y,A,B,C,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4b_4 (Y,A,B,C,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4b base (
        .Y(Y),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4bb (Y,A,B,C_N,D_N);


    
    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    wire nor0_out  ;
    wire and0_out_Y;

    
    nor nor0 (nor0_out  , A, B              );
    and and0 (and0_out_Y, nor0_out, C_N, D_N);
    buf buf0 (Y         , and0_out_Y        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4bb_1 (Y,A,B,C_N,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4bb base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4bb_2 (Y,A,B,C_N,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4bb base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__nor4bb_4 (Y,A,B,C_N,D_N);


    output Y  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__nor4bb base (
        .Y(Y),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2a (X,A1_N,A2_N,B1,B2);


    
    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    wire nand0_out ;
    wire or0_out   ;
    wire and0_out_X;

    
    nand nand0 (nand0_out , A2_N, A1_N        );
    or   or0   (or0_out   , B2, B1            );
    and  and0  (and0_out_X, nand0_out, or0_out);
    buf  buf0  (X         , and0_out_X        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2a_1 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2a base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2a_2 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2a base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2a_4 (X,A1_N,A2_N,B1,B2);


    output X   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2a base (
        .X(X),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2ai (Y,A1_N,A2_N,B1,B2);


    
    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    wire nand0_out  ;
    wire or0_out    ;
    wire nand1_out_Y;

    
    nand nand0 (nand0_out  , A2_N, A1_N        );
    or   or0   (or0_out    , B2, B1            );
    nand nand1 (nand1_out_Y, nand0_out, or0_out);
    buf  buf0  (Y          , nand1_out_Y       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2ai_1 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2ai base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2ai_2 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2ai base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2bb2ai_4 (Y,A1_N,A2_N,B1,B2);


    output Y   ;
    input  A1_N;
    input  A2_N;
    input  B1  ;
    input  B2  ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2bb2ai base (
        .Y(Y),
        .A1_N(A1_N),
        .A2_N(A2_N),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21a (X,A1,A2,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1         );
    and and0 (and0_out_X, or0_out, B1    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21a_1 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21a_2 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21a_4 (X,A1,A2,B1);


    output X ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ai (Y,A1,A2,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A2, A1         );
    nand nand0 (nand0_out_Y, B1, or0_out    );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ai_0 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ai_1 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ai_2 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ai_4 (Y,A1,A2,B1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ba (X,A1,A2,B1_N);


    
    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    wire nor0_out  ;
    wire nor1_out_X;

    
    nor nor0 (nor0_out  , A1, A2         );
    nor nor1 (nor1_out_X, B1_N, nor0_out );
    buf buf0 (X         , nor1_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ba_1 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ba base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ba_2 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ba base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21ba_4 (X,A1,A2,B1_N);


    output X   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21ba base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21bai (Y,A1,A2,B1_N);


    
    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    wire b          ;
    wire or0_out    ;
    wire nand0_out_Y;

    
    not  not0  (b          , B1_N           );
    or   or0   (or0_out    , A2, A1         );
    nand nand0 (nand0_out_Y, b, or0_out     );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21bai_1 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21bai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21bai_2 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21bai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o21bai_4 (Y,A1,A2,B1_N);


    output Y   ;
    input  A1  ;
    input  A2  ;
    input  B1_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o21bai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1_N(B1_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22a (X,A1,A2,B1,B2);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    wire or0_out   ;
    wire or1_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1          );
    or  or1  (or1_out   , B2, B1          );
    and and0 (and0_out_X, or0_out, or1_out);
    buf buf0 (X         , and0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22a_1 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22a_2 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22a_4 (X,A1,A2,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22ai (Y,A1,A2,B1,B2);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    wire nor0_out ;
    wire nor1_out ;
    wire or0_out_Y;

    
    nor nor0 (nor0_out , B1, B2            );
    nor nor1 (nor1_out , A1, A2            );
    or  or0  (or0_out_Y, nor1_out, nor0_out);
    buf buf0 (Y        , or0_out_Y         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22ai_1 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22ai_2 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o22ai_4 (Y,A1,A2,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o22ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31a (X,A1,A2,A3,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1, A3     );
    and and0 (and0_out_X, or0_out, B1    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31a_1 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31a_2 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31a_4 (X,A1,A2,A3,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31ai (Y,A1,A2,A3,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A2, A1, A3     );
    nand nand0 (nand0_out_Y, B1, or0_out    );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31ai_1 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31ai_2 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o31ai_4 (Y,A1,A2,A3,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o31ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32a (X,A1,A2,A3,B1,B2);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    wire or0_out   ;
    wire or1_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1, A3      );
    or  or1  (or1_out   , B2, B1          );
    and and0 (and0_out_X, or0_out, or1_out);
    buf buf0 (X         , and0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32a_1 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32a_2 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32a_4 (X,A1,A2,A3,B1,B2);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32ai (Y,A1,A2,A3,B1,B2);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    wire nor0_out ;
    wire nor1_out ;
    wire or0_out_Y;

    
    nor nor0 (nor0_out , A3, A1, A2        );
    nor nor1 (nor1_out , B1, B2            );
    or  or0  (or0_out_Y, nor1_out, nor0_out);
    buf buf0 (Y        , or0_out_Y         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32ai_1 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32ai_2 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o32ai_4 (Y,A1,A2,A3,B1,B2);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  B2;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o32ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .B2(B2)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41a (X,A1,A2,A3,A4,B1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A4, A3, A2, A1 );
    and and0 (and0_out_X, or0_out, B1    );
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41a_1 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41a_2 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41a_4 (X,A1,A2,A3,A4,B1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41ai (Y,A1,A2,A3,A4,B1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A4, A3, A2, A1 );
    nand nand0 (nand0_out_Y, B1, or0_out    );
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41ai_1 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41ai_2 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o41ai_4 (Y,A1,A2,A3,A4,B1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  A4;
    input  B1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o41ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .A4(A4),
        .B1(B1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211a (X,A1,A2,B1,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1         );
    and and0 (and0_out_X, or0_out, B1, C1);
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211a_1 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211a_2 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211a_4 (X,A1,A2,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211ai (Y,A1,A2,B1,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A2, A1         );
    nand nand0 (nand0_out_Y, C1, or0_out, B1);
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211ai_1 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211ai_2 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o211ai_4 (Y,A1,A2,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o211ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221a (X,A1,A2,B1,B2,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    wire or0_out   ;
    wire or1_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , B2, B1              );
    or  or1  (or1_out   , A2, A1              );
    and and0 (and0_out_X, or0_out, or1_out, C1);
    buf buf0 (X         , and0_out_X          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221a_1 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221a_2 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221a_4 (X,A1,A2,B1,B2,C1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221ai (Y,A1,A2,B1,B2,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    wire or0_out    ;
    wire or1_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , B2, B1              );
    or   or1   (or1_out    , A2, A1              );
    nand nand0 (nand0_out_Y, or1_out, or0_out, C1);
    buf  buf0  (Y          , nand0_out_Y         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221ai_1 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221ai_2 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o221ai_4 (Y,A1,A2,B1,B2,C1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  B2;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o221ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .B2(B2),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311a (X,A1,A2,A3,B1,C1);


    
    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1, A3     );
    and and0 (and0_out_X, or0_out, B1, C1);
    buf buf0 (X         , and0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311a_1 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311a_2 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311a_4 (X,A1,A2,A3,B1,C1);


    output X ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311ai (Y,A1,A2,A3,B1,C1);


    
    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A2, A1, A3     );
    nand nand0 (nand0_out_Y, C1, or0_out, B1);
    buf  buf0  (Y          , nand0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311ai_0 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311ai_1 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311ai_2 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o311ai_4 (Y,A1,A2,A3,B1,C1);


    output Y ;
    input  A1;
    input  A2;
    input  A3;
    input  B1;
    input  C1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o311ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .A3(A3),
        .B1(B1),
        .C1(C1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111a (X,A1,A2,B1,C1,D1);


    
    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    wire or0_out   ;
    wire and0_out_X;

    
    or  or0  (or0_out   , A2, A1             );
    and and0 (and0_out_X, B1, C1, or0_out, D1);
    buf buf0 (X         , and0_out_X         );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111a_1 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111a_2 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111a_4 (X,A1,A2,B1,C1,D1);


    output X ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111a base (
        .X(X),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111ai (Y,A1,A2,B1,C1,D1);


    
    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    wire or0_out    ;
    wire nand0_out_Y;

    
    or   or0   (or0_out    , A2, A1             );
    nand nand0 (nand0_out_Y, C1, B1, D1, or0_out);
    buf  buf0  (Y          , nand0_out_Y        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111ai_1 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111ai_2 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__o2111ai_4 (Y,A1,A2,B1,C1,D1);


    output Y ;
    input  A1;
    input  A2;
    input  B1;
    input  C1;
    input  D1;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__o2111ai base (
        .Y(Y),
        .A1(A1),
        .A2(A2),
        .B1(B1),
        .C1(C1),
        .D1(D1)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2 (X,A,B);


    
    output X;
    input  A;
    input  B;

    
    wire or0_out_X;

    
    or  or0  (or0_out_X, B, A           );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2_0 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2_1 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2_2 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2_4 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2b (X,A,B_N);


    
    output X  ;
    input  A  ;
    input  B_N;

    
    wire not0_out ;
    wire or0_out_X;

    
    not not0 (not0_out , B_N            );
    or  or0  (or0_out_X, not0_out, A    );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2b_1 (X,A,B_N);


    output X  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2b base (
        .X(X),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2b_2 (X,A,B_N);


    output X  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2b base (
        .X(X),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or2b_4 (X,A,B_N);


    output X  ;
    input  A  ;
    input  B_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or2b base (
        .X(X),
        .A(A),
        .B_N(B_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3 (X,A,B,C);


    
    output X;
    input  A;
    input  B;
    input  C;

    
    wire or0_out_X;

    
    or  or0  (or0_out_X, B, A, C        );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3_1 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3_2 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3_4 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3b (X,A,B,C_N);


    
    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    wire not0_out ;
    wire or0_out_X;

    
    not not0 (not0_out , C_N            );
    or  or0  (or0_out_X, B, A, not0_out );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3b_1 (X,A,B,C_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3b base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3b_2 (X,A,B,C_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3b base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or3b_4 (X,A,B,C_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or3b base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4 (X,A,B,C,D);


    
    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    wire or0_out_X;

    
    or  or0  (or0_out_X, D, C, B, A     );
    buf buf0 (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4_1 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4_2 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4_4 (X,A,B,C,D);


    output X;
    input  A;
    input  B;
    input  C;
    input  D;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D(D)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4b (X,A,B,C,D_N);


    
    output X  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    wire not0_out ;
    wire or0_out_X;

    
    not not0 (not0_out , D_N              );
    or  or0  (or0_out_X, not0_out, C, B, A);
    buf buf0 (X        , or0_out_X        );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4b_1 (X,A,B,C,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4b base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4b_2 (X,A,B,C,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4b base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4b_4 (X,A,B,C,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C  ;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4b base (
        .X(X),
        .A(A),
        .B(B),
        .C(C),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4bb (X,A,B,C_N,D_N);


    
    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    wire nand0_out;
    wire or0_out_X;

    
    nand nand0 (nand0_out, D_N, C_N       );
    or   or0   (or0_out_X, B, A, nand0_out);
    buf  buf0  (X        , or0_out_X      );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4bb_1 (X,A,B,C_N,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4bb base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4bb_2 (X,A,B,C_N,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4bb base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__or4bb_4 (X,A,B,C_N,D_N);


    output X  ;
    input  A  ;
    input  B  ;
    input  C_N;
    input  D_N;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__or4bb base (
        .X(X),
        .A(A),
        .B(B),
        .C_N(C_N),
        .D_N(D_N)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__probe_p (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__probe_p_8 (X,A);


    output X;
    input  A;

    
    supply0 VGND;
    supply0 VNB ;
    supply1 VPB ;
    supply1 VPWR;

    sky130_fd_sc_hd__probe_p base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__probec_p (X,A);


    
    output X;
    input  A;

    
    wire buf0_out_X;

    
    buf buf0 (buf0_out_X, A              );
    buf buf1 (X         , buf0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__probec_p_8 (X,A);


    output X;
    input  A;

    
    supply0 VGND;
    supply0 VNB ;
    supply1 VPB ;
    supply1 VPWR;

    sky130_fd_sc_hd__probec_p base (
        .X(X),
        .A(A)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfbbn (Q,Q_N,D,SCD,SCE,CLK_N,SET_B,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    wire RESET  ;
    wire SET    ;
    wire CLK    ;
    wire buf_Q  ;
    wire mux_out;

    
    not                                       not0      (RESET  , RESET_B                 );
    not                                       not1      (SET    , SET_B                   );
    not                                       not2      (CLK    , CLK_N                   );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE             );
    sky130_fd_sc_hd__udp_dff$NSR   dff0      (buf_Q  , SET, RESET, CLK, mux_out);
    buf                                       buf0      (Q      , buf_Q                   );
    not                                       not3      (Q_N    , buf_Q                   );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfbbn_1 (Q,Q_N,D,SCD,SCE,CLK_N,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfbbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .CLK_N(CLK_N),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfbbn_2 (Q,Q_N,D,SCD,SCE,CLK_N,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  CLK_N  ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfbbn base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .CLK_N(CLK_N),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfbbp (Q,Q_N,D,SCD,SCE,CLK,SET_B,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  CLK    ;
    input  SET_B  ;
    input  RESET_B;

    
    wire RESET  ;
    wire SET    ;
    wire buf_Q  ;
    wire mux_out;

    
    not                                       not0      (RESET  , RESET_B                 );
    not                                       not1      (SET    , SET_B                   );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE             );
    sky130_fd_sc_hd__udp_dff$NSR   dff0      (buf_Q  , SET, RESET, CLK, mux_out);
    buf                                       buf0      (Q      , buf_Q                   );
    not                                       not2      (Q_N    , buf_Q                   );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfbbp_1 (Q,Q_N,D,SCD,SCE,CLK,SET_B,RESET_B);


    output Q      ;
    output Q_N    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  CLK    ;
    input  SET_B  ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfbbp base (
        .Q(Q),
        .Q_N(Q_N),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .CLK(CLK),
        .SET_B(SET_B),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrbp (Q,Q_N,CLK,D,SCD,SCE,RESET_B);


    
    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    wire buf_Q  ;
    wire RESET  ;
    wire mux_out;

    
    not                                       not0      (RESET  , RESET_B            );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE        );
    sky130_fd_sc_hd__udp_dff$PR    dff0      (buf_Q  , mux_out, CLK, RESET);
    buf                                       buf0      (Q      , buf_Q              );
    not                                       not1      (Q_N    , buf_Q              );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrbp_1 (Q,Q_N,CLK,D,SCD,SCE,RESET_B);


    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrbp_2 (Q,Q_N,CLK,D,SCD,SCE,RESET_B);


    output Q      ;
    output Q_N    ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtn (Q,CLK_N,D,SCD,SCE,RESET_B);


    
    output Q      ;
    input  CLK_N  ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    wire buf_Q  ;
    wire RESET  ;
    wire intclk ;
    wire mux_out;

    
    not                                       not0      (RESET  , RESET_B               );
    not                                       not1      (intclk , CLK_N                 );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE           );
    sky130_fd_sc_hd__udp_dff$PR    dff0      (buf_Q  , mux_out, intclk, RESET);
    buf                                       buf0      (Q      , buf_Q                 );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtn_1 (Q,CLK_N,D,SCD,SCE,RESET_B);


    output Q      ;
    input  CLK_N  ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrtn base (
        .Q(Q),
        .CLK_N(CLK_N),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtp (Q,CLK,D,SCD,SCE,RESET_B);


    
    output Q      ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    wire buf_Q  ;
    wire RESET  ;
    wire mux_out;

    
    not                                       not0      (RESET  , RESET_B            );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE        );
    sky130_fd_sc_hd__udp_dff$PR    dff0      (buf_Q  , mux_out, CLK, RESET);
    buf                                       buf0      (Q      , buf_Q              );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtp_1 (Q,CLK,D,SCD,SCE,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtp_2 (Q,CLK,D,SCD,SCE,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfrtp_4 (Q,CLK,D,SCD,SCE,RESET_B);


    output Q      ;
    input  CLK    ;
    input  D      ;
    input  SCD    ;
    input  SCE    ;
    input  RESET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfrtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .RESET_B(RESET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfsbp (Q,Q_N,CLK,D,SCD,SCE,SET_B);


    
    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    wire buf_Q  ;
    wire SET    ;
    wire mux_out;

    
    not                                       not0      (SET    , SET_B            );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE      );
    sky130_fd_sc_hd__udp_dff$PS    dff0      (buf_Q  , mux_out, CLK, SET);
    buf                                       buf0      (Q      , buf_Q            );
    not                                       not1      (Q_N    , buf_Q            );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfsbp_1 (Q,Q_N,CLK,D,SCD,SCE,SET_B);


    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfsbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfsbp_2 (Q,Q_N,CLK,D,SCD,SCE,SET_B);


    output Q    ;
    output Q_N  ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfsbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfstp (Q,CLK,D,SCD,SCE,SET_B);


    
    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    wire buf_Q  ;
    wire SET    ;
    wire mux_out;

    
    not                                       not0      (SET    , SET_B            );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE      );
    sky130_fd_sc_hd__udp_dff$PS    dff0      (buf_Q  , mux_out, CLK, SET);
    buf                                       buf0      (Q      , buf_Q            );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfstp_1 (Q,CLK,D,SCD,SCE,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfstp_2 (Q,CLK,D,SCD,SCE,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfstp_4 (Q,CLK,D,SCD,SCE,SET_B);


    output Q    ;
    input  CLK  ;
    input  D    ;
    input  SCD  ;
    input  SCE  ;
    input  SET_B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfstp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE),
        .SET_B(SET_B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxbp (Q,Q_N,CLK,D,SCD,SCE);


    
    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    wire buf_Q  ;
    wire mux_out;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE    );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );
    not                                       not0      (Q_N    , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxbp_1 (Q,Q_N,CLK,D,SCD,SCE);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxbp_2 (Q,Q_N,CLK,D,SCD,SCE);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxtp (Q,CLK,D,SCD,SCE);


    
    output Q  ;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    wire buf_Q  ;
    wire mux_out;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, D, SCD, SCE    );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxtp_1 (Q,CLK,D,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxtp_2 (Q,CLK,D,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdfxtp_4 (Q,CLK,D,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdlclkp (GCLK,SCE,GATE,CLK);


    
    output GCLK;
    input  SCE ;
    input  GATE;
    input  CLK ;

    
    wire m0      ;
    wire m0n     ;
    wire clkn    ;
    wire SCE_GATE;

    
    not                           not0    (m0n     , m0             );
    not                           not1    (clkn    , CLK            );
    nor                           nor0    (SCE_GATE, GATE, SCE      );
    sky130_fd_sc_hd__udp_dlatch$P dlatch0 (m0      , SCE_GATE, clkn );
    and                           and0    (GCLK    , m0n, CLK       );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdlclkp_1 (GCLK,SCE,GATE,CLK);


    output GCLK;
    input  SCE ;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdlclkp base (
        .GCLK(GCLK),
        .SCE(SCE),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdlclkp_2 (GCLK,SCE,GATE,CLK);


    output GCLK;
    input  SCE ;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdlclkp base (
        .GCLK(GCLK),
        .SCE(SCE),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sdlclkp_4 (GCLK,SCE,GATE,CLK);


    output GCLK;
    input  SCE ;
    input  GATE;
    input  CLK ;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sdlclkp base (
        .GCLK(GCLK),
        .SCE(SCE),
        .GATE(GATE),
        .CLK(CLK)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxbp (Q,Q_N,CLK,D,DE,SCD,SCE);


    
    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    wire buf_Q  ;
    wire mux_out;
    wire de_d   ;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, de_d, SCD, SCE );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to11 (de_d   , buf_Q, D, DE   );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );
    not                                       not0      (Q_N    , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxbp_1 (Q,Q_N,CLK,D,DE,SCD,SCE);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sedfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .DE(DE),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxbp_2 (Q,Q_N,CLK,D,DE,SCD,SCE);


    output Q  ;
    output Q_N;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sedfxbp base (
        .Q(Q),
        .Q_N(Q_N),
        .CLK(CLK),
        .D(D),
        .DE(DE),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxtp (Q,CLK,D,DE,SCD,SCE);


    
    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    wire buf_Q  ;
    wire mux_out;
    wire de_d   ;

    
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to10 (mux_out, de_d, SCD, SCE );
    sky130_fd_sc_hd__udp_mux_2to1             mux_2to11 (de_d   , buf_Q, D, DE   );
    sky130_fd_sc_hd__udp_dff$P     dff0      (buf_Q  , mux_out, CLK   );
    buf                                       buf0      (Q      , buf_Q          );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxtp_1 (Q,CLK,D,DE,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sedfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .DE(DE),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxtp_2 (Q,CLK,D,DE,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sedfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .DE(DE),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__sedfxtp_4 (Q,CLK,D,DE,SCD,SCE);


    output Q  ;
    input  CLK;
    input  D  ;
    input  DE ;
    input  SCD;
    input  SCE;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__sedfxtp base (
        .Q(Q),
        .CLK(CLK),
        .D(D),
        .DE(DE),
        .SCD(SCD),
        .SCE(SCE)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__tap ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__tap_1 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__tap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__tap_2 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__tap base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvgnd2 ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvgnd2_1 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__tapvgnd2 base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvgnd ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvgnd_1 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__tapvgnd base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvpwrvgnd ();

     

endmodule
(* noblackbox *) module sky130_fd_sc_hd__tapvpwrvgnd_1 ();

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__tapvpwrvgnd base ();


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor2 (Y,A,B);


    
    output Y;
    input  A;
    input  B;

    
    wire xnor0_out_Y;

    
    xnor xnor0 (xnor0_out_Y, A, B           );
    buf  buf0  (Y          , xnor0_out_Y    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor2_1 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor2_2 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor2_4 (Y,A,B);


    output Y;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor2 base (
        .Y(Y),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor3 (X,A,B,C);


    
    output X;
    input  A;
    input  B;
    input  C;

    
    wire xnor0_out_X;

    
    xnor xnor0 (xnor0_out_X, A, B, C        );
    buf  buf0  (X          , xnor0_out_X    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor3_1 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor3_2 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xnor3_4 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xnor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor2 (X,A,B);


    
    output X;
    input  A;
    input  B;

    
    wire xor0_out_X;

    
    xor xor0 (xor0_out_X, B, A           );
    buf buf0 (X         , xor0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor2_1 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor2_2 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor2_4 (X,A,B);


    output X;
    input  A;
    input  B;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor2 base (
        .X(X),
        .A(A),
        .B(B)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor3 (X,A,B,C);


    
    output X;
    input  A;
    input  B;
    input  C;

    
    wire xor0_out_X;

    
    xor xor0 (xor0_out_X, A, B, C        );
    buf buf0 (X         , xor0_out_X     );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor3_1 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor3_2 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module sky130_fd_sc_hd__xor3_4 (X,A,B,C);


    output X;
    input  A;
    input  B;
    input  C;

    
    supply1 VPWR;
    supply0 VGND;
    supply1 VPB ;
    supply0 VNB ;

    sky130_fd_sc_hd__xor3 base (
        .X(X),
        .A(A),
        .B(B),
        .C(C)
    );


endmodule
(* noblackbox *) module mod_pullup (Y);
output Y;
assign Y = 1'b1;
endmodule
(* noblackbox *) module mod_pulldown (Y);
output Y;
assign Y = 1'b0;
endmodule
