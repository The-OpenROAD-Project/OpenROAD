module top(clk, rst, \q1[0] , \q1[1] );
  wire _00_;
  wire _01_;
  wire _02_;
  wire _03_;
  wire _04_;
  wire _05_;
  wire _06_;
  wire _07_;
  wire _08_;
  input clk;
  wire clk;
  output \q1[0] ;
  wire \q1[0] ;
  output \q1[1] ;
  wire \q1[1] ;
  wire \q2[0] ;
  wire \q2[1] ;
  wire \q3[0] ;
  wire \q3[1] ;
  input rst;
  wire rst;
  sky130_fd_sc_hd__xnor2_1 _09_ (
    .A(\q1[0] ),
    .B(\q1[1] ),
    .Y(_06_)
  );
  sky130_fd_sc_hd__nor2_1 _10_ (
    .A(rst),
    .B(\q1[0] ),
    .Y(_05_)
  );
  sky130_fd_sc_hd__nor2_1 _11_ (
    .A(rst),
    .B(_06_),
    .Y(_00_)
  );
  sky130_fd_sc_hd__xnor2_1 _12_ (
    .A(\q2[0] ),
    .B(\q2[1] ),
    .Y(_07_)
  );
  sky130_fd_sc_hd__nor2_1 _13_ (
    .A(rst),
    .B(_07_),
    .Y(_01_)
  );
  sky130_fd_sc_hd__xor2_1 _14_ (
    .A(\q3[0] ),
    .B(\q3[1] ),
    .X(_08_)
  );
  sky130_fd_sc_hd__lpflow_inputiso1p_1 _15_ (
    .A(rst),
    .SLEEP(_08_),
    .X(_02_)
  );
  sky130_fd_sc_hd__nor2_1 _16_ (
    .A(rst),
    .B(\q3[0] ),
    .Y(_03_)
  );
  sky130_fd_sc_hd__nand2b_1 _17_ (
    .A_N(rst),
    .B(\q2[0] ),
    .Y(_04_)
  );
  sky130_fd_sc_hd__dfxtp_1 _18_ (
    .CLK(clk),
    .D(_00_),
    .Q(\q1[1] )
  );
  sky130_fd_sc_hd__dfxtp_1 _19_ (
    .CLK(clk),
    .D(_01_),
    .Q(\q2[1] )
  );
  sky130_fd_sc_hd__dfxtp_1 _20_ (
    .CLK(clk),
    .D(_02_),
    .Q(\q3[1] )
  );
  sky130_fd_sc_hd__dfxtp_1 _21_ (
    .CLK(clk),
    .D(_03_),
    .Q(\q3[0] )
  );
  sky130_fd_sc_hd__dfxtp_1 _22_ (
    .CLK(clk),
    .D(_04_),
    .Q(\q2[0] )
  );
  sky130_fd_sc_hd__dfxtp_1 _23_ (
    .CLK(clk),
    .D(_05_),
    .Q(\q1[0] )
  );
endmodule
