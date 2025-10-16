module top(clk, y, load);
  wire _00_;
  wire _01_;
  wire _02_;
  wire _03_;
  wire _04_;
  wire _05_;
  wire _06_;
  wire _07_;
  wire _08_;
  wire _09_;
  wire _10_;
  wire _11_;
  wire _12_;
  wire _13_;
  wire _14_;
  wire _15_;
  wire _16_;
  wire _17_;
  wire _18_;
  wire _19_;
  wire _20_;
  wire _21_;
  wire _22_;
  wire _23_;
  wire _24_;
  wire _25_;
  wire _26_;
  wire _27_;
  wire _28_;
  wire _29_;
  wire _30_;
  wire _31_;
  wire _32_;
  wire _33_;
  wire _34_;
  wire _35_;
  wire _36_;
  (* force_downto = 32'd1 *)
  wire [7:0] _37_;
  (* force_downto = 32'd1 *)
  wire [7:0] _38_;
  wire _39_;
  wire _40_;
  wire _41_;
  wire _42_;
  wire _43_;
  wire _44_;
  wire _45_;
  wire _46_;
  input clk;
  wire clk;
  input load;
  wire load;
  output [7:0] y;
  wire [7:0] y;
  sky130_fd_sc_hd__and3_1 _47_ (
    .A(_29_),
    .B(_30_),
    .C(_31_),
    .X(_17_)
  );
  sky130_fd_sc_hd__and4_1 _48_ (
    .A(_29_),
    .B(_30_),
    .C(_31_),
    .D(_32_),
    .X(_18_)
  );
  sky130_fd_sc_hd__and2_0 _49_ (
    .A(_33_),
    .B(_18_),
    .X(_19_)
  );
  sky130_fd_sc_hd__and3_1 _50_ (
    .A(_33_),
    .B(_34_),
    .C(_18_),
    .X(_20_)
  );
  sky130_fd_sc_hd__o21bai_1 _51_ (
    .A1(_35_),
    .A2(_20_),
    .B1_N(_16_),
    .Y(_21_)
  );
  sky130_fd_sc_hd__a21oi_1 _52_ (
    .A1(_35_),
    .A2(_20_),
    .B1(_21_),
    .Y(_08_)
  );
  sky130_fd_sc_hd__and3_1 _53_ (
    .A(_35_),
    .B(_36_),
    .C(_20_),
    .X(_22_)
  );
  sky130_fd_sc_hd__a21oi_1 _54_ (
    .A1(_35_),
    .A2(_20_),
    .B1(_36_),
    .Y(_23_)
  );
  sky130_fd_sc_hd__nor3_1 _55_ (
    .A(_16_),
    .B(_22_),
    .C(_23_),
    .Y(_09_)
  );
  sky130_fd_sc_hd__nor2_1 _56_ (
    .A(_29_),
    .B(_16_),
    .Y(_10_)
  );
  sky130_fd_sc_hd__a21oi_1 _57_ (
    .A1(_29_),
    .A2(_30_),
    .B1(_16_),
    .Y(_24_)
  );
  sky130_fd_sc_hd__o21a_1 _58_ (
    .A1(_29_),
    .A2(_30_),
    .B1(_24_),
    .X(_11_)
  );
  sky130_fd_sc_hd__a21oi_1 _59_ (
    .A1(_29_),
    .A2(_30_),
    .B1(_31_),
    .Y(_25_)
  );
  sky130_fd_sc_hd__nor3_1 _60_ (
    .A(_16_),
    .B(_17_),
    .C(_25_),
    .Y(_12_)
  );
  sky130_fd_sc_hd__nor2_1 _61_ (
    .A(_32_),
    .B(_17_),
    .Y(_26_)
  );
  sky130_fd_sc_hd__nor3_1 _62_ (
    .A(_16_),
    .B(_18_),
    .C(_26_),
    .Y(_13_)
  );
  sky130_fd_sc_hd__nor2_1 _63_ (
    .A(_33_),
    .B(_18_),
    .Y(_27_)
  );
  sky130_fd_sc_hd__nor3_1 _64_ (
    .A(_16_),
    .B(_19_),
    .C(_27_),
    .Y(_14_)
  );
  sky130_fd_sc_hd__nor2_1 _65_ (
    .A(_34_),
    .B(_19_),
    .Y(_28_)
  );
  sky130_fd_sc_hd__nor3_1 _66_ (
    .A(_16_),
    .B(_20_),
    .C(_28_),
    .Y(_15_)
  );
  sky130_fd_sc_hd__dfxtp_1 _67_ (
    .CLK(clk),
    .D(_39_),
    .Q(y[6])
  );
  sky130_fd_sc_hd__dfxtp_1 _68_ (
    .CLK(clk),
    .D(_40_),
    .Q(y[7])
  );
  sky130_fd_sc_hd__dfxtp_1 _69_ (
    .CLK(clk),
    .D(_41_),
    .Q(y[0])
  );
  sky130_fd_sc_hd__dfxtp_1 _70_ (
    .CLK(clk),
    .D(_42_),
    .Q(y[1])
  );
  sky130_fd_sc_hd__dfxtp_1 _71_ (
    .CLK(clk),
    .D(_43_),
    .Q(y[2])
  );
  sky130_fd_sc_hd__dfxtp_1 _72_ (
    .CLK(clk),
    .D(_44_),
    .Q(y[3])
  );
  sky130_fd_sc_hd__dfxtp_1 _73_ (
    .CLK(clk),
    .D(_45_),
    .Q(y[4])
  );
  sky130_fd_sc_hd__dfxtp_1 _74_ (
    .CLK(clk),
    .D(_46_),
    .Q(y[5])
  );
  assign _37_[7:1] = y[7:1];
  assign _38_[0] = _37_[0];
  assign _29_ = y[0];
  assign _30_ = y[1];
  assign _31_ = y[2];
  assign _32_ = y[3];
  assign _33_ = y[4];
  assign _34_ = y[5];
  assign _35_ = y[6];
  assign _36_ = y[7];
  assign _16_ = load;
  assign _39_ = _08_;
  assign _40_ = _09_;
  assign _41_ = _10_;
  assign _42_ = _11_;
  assign _43_ = _12_;
  assign _44_ = _13_;
  assign _45_ = _14_;
  assign _46_ = _15_;
endmodule
