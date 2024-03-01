module hierclock (a_count_valid_o,
    a_ld_i,
    b_count_valid_o,
    b_ld_i,
    clk_i,
    rst_n_i,
    a_count_o,
    a_i,
    b_count_o,
    b_i);
 output a_count_valid_o;
 input a_ld_i;
 output b_count_valid_o;
 input b_ld_i;
 input clk_i;
 input rst_n_i;
 output [3:0] a_count_o;
 input [3:0] a_i;
 output [3:0] b_count_o;
 input [3:0] b_i;

 wire clk1_int;
 wire clk2_int;
 wire \U1/_03_ ;
 wire \U1/_04_ ;
 wire \U1/_05_ ;
 wire \U1/_06_ ;
 wire \U1/_11_ ;
 wire \U1/_12_ ;
 wire \U1/_13_ ;
 wire \U1/_14_ ;
 wire \U1/_15_ ;
 wire \U1/_19_ ;
 wire \U1/_20_ ;
 wire \U1/_21_ ;
 wire \U1/_22_ ;
 wire \U2/_12_ ;
 wire \U2/_13_ ;
 wire \U2/_14_ ;
 wire \U2/_15_ ;
 wire \U2/_16_ ;
 wire \U2/_26_ ;
 wire \U2/_27_ ;
 wire \U2/_28_ ;
 wire \U2/_29_ ;
 wire \U2/_30_ ;
 wire \U2/_31_ ;
 wire \U2/_32_ ;
 wire \U2/_33_ ;
 wire \U2/_34_ ;
 wire \U2/_35_ ;
 wire \U2/_36_ ;
 wire \U2/_38_ ;
 wire \U2/_39_ ;
 wire \U2/_40_ ;
 wire \U2/_41_ ;
 wire \U2/_42_ ;
 wire \U3/_12_ ;
 wire \U3/_13_ ;
 wire \U3/_14_ ;
 wire \U3/_15_ ;
 wire \U3/_16_ ;
 wire \U3/_26_ ;
 wire \U3/_27_ ;
 wire \U3/_28_ ;
 wire \U3/_29_ ;
 wire \U3/_30_ ;
 wire \U3/_31_ ;
 wire \U3/_32_ ;
 wire \U3/_33_ ;
 wire \U3/_34_ ;
 wire \U3/_35_ ;
 wire \U3/_36_ ;
 wire \U3/_38_ ;
 wire \U3/_39_ ;
 wire \U3/_40_ ;
 wire \U3/_41_ ;
 wire \U3/_42_ ;
 wire [2:0] \U1/counter_q ;

 clockgen U1 (.clk2_o(clk2_int),
    .clk1_o(clk1_int),
    .rst_n_i(rst_n_i),
    .clk_i(clk_i));
 counter U2 (.count_valid_o(a_count_valid_o),
    .load_i(a_ld_i),
    .rst_n_i(rst_n_i),
    .clk_i(clk1_int),
    .count_value_o({a_count_o[3],
    a_count_o[2],
    a_count_o[1],
    a_count_o[0]}),
    .load_value_i({a_i[3],
    a_i[2],
    a_i[1],
    a_i[0]}));
 counter-1 U3 (.count_valid_o(b_count_valid_o),
    .load_i(b_ld_i),
    .rst_n_i(rst_n_i),
    .clk_i(clk2_int),
    .count_value_o({b_count_o[3],
    b_count_o[2],
    b_count_o[1],
    b_count_o[0]}),
    .load_value_i({b_i[3],
    b_i[2],
    b_i[1],
    b_i[0]}));
endmodule
module clockgen (clk2_o,
    clk1_o,
    rst_n_i,
    clk_i);
 output clk2_o;
 output clk1_o;
 input rst_n_i;
 input clk_i;


 DFF_X1 \U1/_40_  (.D(_06_),
    .CK(clk_i),
    .Q(counter_q[3]),
    .QN(_19_));
 DFF_X1 \U1/_39_  (.D(_05_),
    .CK(clk_i),
    .Q(counter_q[2]),
    .QN(_20_));
 DFF_X2 \U1/_38_  (.D(_04_),
    .CK(clk_i),
    .Q(counter_q[1]),
    .QN(_21_));
 DFF_X1 \U1/_37_  (.D(_03_),
    .CK(clk_i),
    .Q(counter_q[0]),
    .QN(_22_));
 NOR2_X1 \U1/_36_  (.A1(_11_),
    .A2(_15_),
    .ZN(_06_));
 XNOR2_X1 \U1/_35_  (.A(counter_q[3]),
    .B(_13_),
    .ZN(_15_));
 NOR3_X1 \U1/_34_  (.A1(_11_),
    .A2(_13_),
    .A3(_14_),
    .ZN(_05_));
 AOI21_X1 \U1/_33_  (.A(counter_q[2]),
    .B1(counter_q[1]),
    .B2(counter_q[0]),
    .ZN(_14_));
 AND3_X1 \U1/_32_  (.A1(counter_q[0]),
    .A2(counter_q[1]),
    .A3(counter_q[2]),
    .ZN(_13_));
 AOI21_X1 \U1/_31_  (.A(_12_),
    .B1(counter_q[1]),
    .B2(counter_q[0]),
    .ZN(_04_));
 OAI21_X1 \U1/_30_  (.A(rst_n_i),
    .B1(counter_q[1]),
    .B2(counter_q[0]),
    .ZN(_12_));
 AND2_X1 \U1/_29_  (.A1(_22_),
    .A2(rst_n_i),
    .ZN(_03_));
 INV_X1 \U1/_28_  (.A(rst_n_i),
    .ZN(_11_));
 assign clk2_o = counter_q[3];
 assign clk1_o = counter_q[1];
endmodule
module counter (count_valid_o,
    load_i,
    rst_n_i,
    clk_i,
    count_value_o,
    load_value_i);
 output count_valid_o;
 input load_i;
 input rst_n_i;
 input clk_i;
 output [3:0] count_value_o;
 input [3:0] load_value_i;


 DFF_X1 \U2/_69_  (.D(_16_),
    .CK(clk_i),
    .Q(counter_q[3]),
    .QN(_38_));
 DFF_X1 \U2/_68_  (.D(_15_),
    .CK(clk_i),
    .Q(counter_q[2]),
    .QN(_39_));
 DFF_X1 \U2/_67_  (.D(_14_),
    .CK(clk_i),
    .Q(counter_q[1]),
    .QN(_40_));
 DFF_X1 \U2/_66_  (.D(_13_),
    .CK(clk_i),
    .Q(counter_q[0]),
    .QN(_41_));
 DFF_X1 \U2/_65_  (.D(_12_),
    .CK(clk_i),
    .Q(count_valid_q),
    .QN(_42_));
 AOI21_X1 \U2/_64_  (.A(_36_),
    .B1(_35_),
    .B2(_27_),
    .ZN(_16_));
 OAI21_X1 \U2/_63_  (.A(rst_n_i),
    .B1(load_value_i[3]),
    .B2(_27_),
    .ZN(_36_));
 XOR2_X1 \U2/_62_  (.A(counter_q[3]),
    .B(_32_),
    .Z(_35_));
 AOI21_X1 \U2/_61_  (.A(_34_),
    .B1(_33_),
    .B2(_27_),
    .ZN(_15_));
 OAI21_X1 \U2/_60_  (.A(rst_n_i),
    .B1(load_value_i[2]),
    .B2(_27_),
    .ZN(_34_));
 XOR2_X1 \U2/_59_  (.A(counter_q[2]),
    .B(_29_),
    .Z(_33_));
 NAND3_X1 \U2/_58_  (.A1(counter_q[0]),
    .A2(counter_q[1]),
    .A3(counter_q[2]),
    .ZN(_32_));
 AOI21_X1 \U2/_57_  (.A(_31_),
    .B1(_30_),
    .B2(_27_),
    .ZN(_14_));
 OAI21_X1 \U2/_56_  (.A(rst_n_i),
    .B1(load_value_i[1]),
    .B2(_27_),
    .ZN(_31_));
 XNOR2_X1 \U2/_55_  (.A(counter_q[0]),
    .B(counter_q[1]),
    .ZN(_30_));
 NAND2_X1 \U2/_54_  (.A1(counter_q[0]),
    .A2(counter_q[1]),
    .ZN(_29_));
 AOI21_X1 \U2/_53_  (.A(_28_),
    .B1(_27_),
    .B2(_26_),
    .ZN(_13_));
 OAI21_X1 \U2/_52_  (.A(rst_n_i),
    .B1(_27_),
    .B2(load_value_i[0]),
    .ZN(_28_));
 AND2_X1 \U2/_51_  (.A1(_27_),
    .A2(rst_n_i),
    .ZN(_12_));
 INV_X2 \U2/_50_  (.A(load_i),
    .ZN(_27_));
 INV_X1 \U2/_49_  (.A(_41_),
    .ZN(_26_));
 assign count_valid_o = count_valid_q;
 assign count_value_o[0] = counter_q[0];
 assign count_value_o[1] = counter_q[1];
 assign count_value_o[2] = counter_q[2];
 assign count_value_o[3] = counter_q[3];
endmodule
module counter-1 (count_valid_o,
    load_i,
    rst_n_i,
    clk_i,
    count_value_o,
    load_value_i);
 output count_valid_o;
 input load_i;
 input rst_n_i;
 input clk_i;
 output [3:0] count_value_o;
 input [3:0] load_value_i;


 DFF_X1 \U3/_69_  (.D(_16_),
    .CK(clk_i),
    .Q(counter_q[3]),
    .QN(_38_));
 DFF_X1 \U3/_68_  (.D(_15_),
    .CK(clk_i),
    .Q(counter_q[2]),
    .QN(_39_));
 DFF_X1 \U3/_67_  (.D(_14_),
    .CK(clk_i),
    .Q(counter_q[1]),
    .QN(_40_));
 DFF_X1 \U3/_66_  (.D(_13_),
    .CK(clk_i),
    .Q(counter_q[0]),
    .QN(_41_));
 DFF_X1 \U3/_65_  (.D(_12_),
    .CK(clk_i),
    .Q(count_valid_q),
    .QN(_42_));
 AOI21_X1 \U3/_64_  (.A(_36_),
    .B1(_35_),
    .B2(_27_),
    .ZN(_16_));
 OAI21_X1 \U3/_63_  (.A(rst_n_i),
    .B1(load_value_i[3]),
    .B2(_27_),
    .ZN(_36_));
 XOR2_X1 \U3/_62_  (.A(counter_q[3]),
    .B(_32_),
    .Z(_35_));
 AOI21_X1 \U3/_61_  (.A(_34_),
    .B1(_33_),
    .B2(_27_),
    .ZN(_15_));
 OAI21_X1 \U3/_60_  (.A(rst_n_i),
    .B1(load_value_i[2]),
    .B2(_27_),
    .ZN(_34_));
 XOR2_X1 \U3/_59_  (.A(counter_q[2]),
    .B(_29_),
    .Z(_33_));
 NAND3_X1 \U3/_58_  (.A1(counter_q[0]),
    .A2(counter_q[1]),
    .A3(counter_q[2]),
    .ZN(_32_));
 AOI21_X1 \U3/_57_  (.A(_31_),
    .B1(_30_),
    .B2(_27_),
    .ZN(_14_));
 OAI21_X1 \U3/_56_  (.A(rst_n_i),
    .B1(load_value_i[1]),
    .B2(_27_),
    .ZN(_31_));
 XNOR2_X1 \U3/_55_  (.A(counter_q[0]),
    .B(counter_q[1]),
    .ZN(_30_));
 NAND2_X1 \U3/_54_  (.A1(counter_q[0]),
    .A2(counter_q[1]),
    .ZN(_29_));
 AOI21_X1 \U3/_53_  (.A(_28_),
    .B1(_27_),
    .B2(_26_),
    .ZN(_13_));
 OAI21_X1 \U3/_52_  (.A(rst_n_i),
    .B1(_27_),
    .B2(load_value_i[0]),
    .ZN(_28_));
 AND2_X1 \U3/_51_  (.A1(_27_),
    .A2(rst_n_i),
    .ZN(_12_));
 INV_X2 \U3/_50_  (.A(load_i),
    .ZN(_27_));
 INV_X1 \U3/_49_  (.A(_41_),
    .ZN(_26_));
 assign count_valid_o = count_valid_q;
 assign count_value_o[0] = counter_q[0];
 assign count_value_o[1] = counter_q[1];
 assign count_value_o[2] = counter_q[2];
 assign count_value_o[3] = counter_q[3];
endmodule
