module tempsenseInst (CLK_REF,
    DONE,
    RESET_COUNTERn,
    en,
    lc_out,
    out,
    outb,
    DOUT,
    SEL_CONV_TIME);
 input CLK_REF;
 output DONE;
 input RESET_COUNTERn;
 input en;
 output lc_out;
 output out;
 output outb;
 output [23:0] DOUT;
 input [3:0] SEL_CONV_TIME;

 wire nPWRUP0;
 wire nPWRUP1;
 wire _000_;
 wire _001_;
 wire _002_;
 wire _003_;
 wire _004_;
 wire _005_;
 wire _006_;
 wire _007_;
 wire _008_;
 wire _009_;
 wire _010_;
 wire _011_;
 wire _012_;
 wire _013_;
 wire _014_;
 wire _015_;
 wire _016_;
 wire _017_;
 wire _018_;
 wire _019_;
 wire _020_;
 wire _021_;
 wire _022_;
 wire _023_;
 wire _024_;
 wire _025_;
 wire _026_;
 wire _027_;
 wire _028_;
 wire _029_;
 wire _030_;
 wire _031_;
 wire _032_;
 wire _033_;
 wire _034_;
 wire _035_;
 wire _036_;
 wire _037_;
 wire _038_;
 wire _039_;
 wire _040_;
 wire _041_;
 wire _042_;
 wire _043_;
 wire _044_;
 wire _045_;
 wire _048_;
 wire _049_;
 wire _050_;
 wire _051_;
 wire _052_;
 wire _053_;
 wire _054_;
 wire _055_;
 wire _057_;
 wire _058_;
 wire _059_;
 wire _060_;
 wire _061_;
 wire _062_;
 wire _067_;
 wire _068_;
 wire \temp_analog_0.n1 ;
 wire \temp_analog_0.n2 ;
 wire \temp_analog_0.n3 ;
 wire \temp_analog_0.n4 ;
 wire \temp_analog_0.n5 ;
 wire \temp_analog_0.n6 ;
 wire \temp_analog_0.n7 ;
 wire \temp_analog_0.nx2 ;
 wire \temp_analog_1.VIN ;
 wire \temp_analog_1.async_counter_0.WAKE ;
 wire \temp_analog_1.async_counter_0.WAKE_pre ;
 wire \temp_analog_1.async_counter_0.clk_ref_in ;
 wire \temp_analog_1.async_counter_0.clk_sens_in ;
 wire \temp_analog_1.async_counter_0.div_r[0] ;
 wire \temp_analog_1.async_counter_0.div_r[10] ;
 wire \temp_analog_1.async_counter_0.div_r[11] ;
 wire \temp_analog_1.async_counter_0.div_r[12] ;
 wire \temp_analog_1.async_counter_0.div_r[13] ;
 wire \temp_analog_1.async_counter_0.div_r[14] ;
 wire \temp_analog_1.async_counter_0.div_r[15] ;
 wire \temp_analog_1.async_counter_0.div_r[16] ;
 wire \temp_analog_1.async_counter_0.div_r[17] ;
 wire \temp_analog_1.async_counter_0.div_r[18] ;
 wire \temp_analog_1.async_counter_0.div_r[19] ;
 wire \temp_analog_1.async_counter_0.div_r[1] ;
 wire \temp_analog_1.async_counter_0.div_r[20] ;
 wire \temp_analog_1.async_counter_0.div_r[2] ;
 wire \temp_analog_1.async_counter_0.div_r[3] ;
 wire \temp_analog_1.async_counter_0.div_r[4] ;
 wire \temp_analog_1.async_counter_0.div_r[5] ;
 wire \temp_analog_1.async_counter_0.div_r[6] ;
 wire \temp_analog_1.async_counter_0.div_r[7] ;
 wire \temp_analog_1.async_counter_0.div_r[8] ;
 wire \temp_analog_1.async_counter_0.div_r[9] ;
 wire \temp_analog_1.async_counter_0.div_s[0] ;
 wire \temp_analog_1.async_counter_0.div_s[10] ;
 wire \temp_analog_1.async_counter_0.div_s[11] ;
 wire \temp_analog_1.async_counter_0.div_s[12] ;
 wire \temp_analog_1.async_counter_0.div_s[13] ;
 wire \temp_analog_1.async_counter_0.div_s[14] ;
 wire \temp_analog_1.async_counter_0.div_s[15] ;
 wire \temp_analog_1.async_counter_0.div_s[16] ;
 wire \temp_analog_1.async_counter_0.div_s[17] ;
 wire \temp_analog_1.async_counter_0.div_s[18] ;
 wire \temp_analog_1.async_counter_0.div_s[19] ;
 wire \temp_analog_1.async_counter_0.div_s[1] ;
 wire \temp_analog_1.async_counter_0.div_s[20] ;
 wire \temp_analog_1.async_counter_0.div_s[21] ;
 wire \temp_analog_1.async_counter_0.div_s[22] ;
 wire \temp_analog_1.async_counter_0.div_s[23] ;
 wire \temp_analog_1.async_counter_0.div_s[2] ;
 wire \temp_analog_1.async_counter_0.div_s[3] ;
 wire \temp_analog_1.async_counter_0.div_s[4] ;
 wire \temp_analog_1.async_counter_0.div_s[5] ;
 wire \temp_analog_1.async_counter_0.div_s[6] ;
 wire \temp_analog_1.async_counter_0.div_s[7] ;
 wire \temp_analog_1.async_counter_0.div_s[8] ;
 wire \temp_analog_1.async_counter_0.div_s[9] ;

 sky130_fd_sc_hd__decap_4 PHY_0 ();
 sky130_fd_sc_hd__decap_4 PHY_1 ();
 sky130_fd_sc_hd__decap_4 PHY_10 ();
 sky130_fd_sc_hd__decap_4 PHY_100 ();
 sky130_fd_sc_hd__decap_4 PHY_101 ();
 sky130_fd_sc_hd__decap_4 PHY_102 ();
 sky130_fd_sc_hd__decap_4 PHY_103 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_104 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_105 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_106 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_107 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_108 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_109 ();
 sky130_fd_sc_hd__decap_4 PHY_11 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_110 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_111 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_112 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_113 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_114 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_115 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_116 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_117 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_118 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_119 ();
 sky130_fd_sc_hd__decap_4 PHY_12 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_120 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_121 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_122 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_123 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_124 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_125 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_126 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_127 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_128 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_129 ();
 sky130_fd_sc_hd__decap_4 PHY_13 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_130 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_131 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_132 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_133 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_134 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_135 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_136 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_137 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_138 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_139 ();
 sky130_fd_sc_hd__decap_4 PHY_14 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_140 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_141 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_142 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_143 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_144 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_145 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_146 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_147 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_148 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_149 ();
 sky130_fd_sc_hd__decap_4 PHY_15 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_150 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_151 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_152 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_153 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_154 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_155 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_156 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_157 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_158 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_159 ();
 sky130_fd_sc_hd__decap_4 PHY_16 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_160 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_161 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_162 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_163 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_164 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_165 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_166 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_167 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_168 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_169 ();
 sky130_fd_sc_hd__decap_4 PHY_17 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_170 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_171 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_172 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_173 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_174 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_175 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_176 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_177 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_178 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_179 ();
 sky130_fd_sc_hd__decap_4 PHY_18 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_180 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_181 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_182 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_183 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_184 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_185 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_186 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_187 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_188 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_189 ();
 sky130_fd_sc_hd__decap_4 PHY_19 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_190 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_191 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_192 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_193 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_194 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_195 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_196 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_197 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_198 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_199 ();
 sky130_fd_sc_hd__decap_4 PHY_2 ();
 sky130_fd_sc_hd__decap_4 PHY_20 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_200 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_201 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_202 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_203 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_204 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_205 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_206 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_207 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_208 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_209 ();
 sky130_fd_sc_hd__decap_4 PHY_21 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_210 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_211 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_212 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_213 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_214 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_215 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_216 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_217 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_218 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_219 ();
 sky130_fd_sc_hd__decap_4 PHY_22 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_220 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_221 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_222 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_223 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_224 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_225 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_226 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_227 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_228 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_229 ();
 sky130_fd_sc_hd__decap_4 PHY_23 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_230 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_231 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_232 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_233 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_234 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_235 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_236 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_237 ();
 sky130_fd_sc_hd__tapvpwrvgnd_1 PHY_238 ();
 sky130_fd_sc_hd__decap_4 PHY_24 ();
 sky130_fd_sc_hd__decap_4 PHY_25 ();
 sky130_fd_sc_hd__decap_4 PHY_26 ();
 sky130_fd_sc_hd__decap_4 PHY_27 ();
 sky130_fd_sc_hd__decap_4 PHY_28 ();
 sky130_fd_sc_hd__decap_4 PHY_29 ();
 sky130_fd_sc_hd__decap_4 PHY_3 ();
 sky130_fd_sc_hd__decap_4 PHY_30 ();
 sky130_fd_sc_hd__decap_4 PHY_31 ();
 sky130_fd_sc_hd__decap_4 PHY_32 ();
 sky130_fd_sc_hd__decap_4 PHY_33 ();
 sky130_fd_sc_hd__decap_4 PHY_34 ();
 sky130_fd_sc_hd__decap_4 PHY_35 ();
 sky130_fd_sc_hd__decap_4 PHY_36 ();
 sky130_fd_sc_hd__decap_4 PHY_37 ();
 sky130_fd_sc_hd__decap_4 PHY_38 ();
 sky130_fd_sc_hd__decap_4 PHY_39 ();
 sky130_fd_sc_hd__decap_4 PHY_4 ();
 sky130_fd_sc_hd__decap_4 PHY_40 ();
 sky130_fd_sc_hd__decap_4 PHY_41 ();
 sky130_fd_sc_hd__decap_4 PHY_42 ();
 sky130_fd_sc_hd__decap_4 PHY_43 ();
 sky130_fd_sc_hd__decap_4 PHY_44 ();
 sky130_fd_sc_hd__decap_4 PHY_45 ();
 sky130_fd_sc_hd__decap_4 PHY_46 ();
 sky130_fd_sc_hd__decap_4 PHY_47 ();
 sky130_fd_sc_hd__decap_4 PHY_48 ();
 sky130_fd_sc_hd__decap_4 PHY_49 ();
 sky130_fd_sc_hd__decap_4 PHY_5 ();
 sky130_fd_sc_hd__decap_4 PHY_50 ();
 sky130_fd_sc_hd__decap_4 PHY_51 ();
 sky130_fd_sc_hd__decap_4 PHY_52 ();
 sky130_fd_sc_hd__decap_4 PHY_53 ();
 sky130_fd_sc_hd__decap_4 PHY_54 ();
 sky130_fd_sc_hd__decap_4 PHY_55 ();
 sky130_fd_sc_hd__decap_4 PHY_56 ();
 sky130_fd_sc_hd__decap_4 PHY_57 ();
 sky130_fd_sc_hd__decap_4 PHY_58 ();
 sky130_fd_sc_hd__decap_4 PHY_59 ();
 sky130_fd_sc_hd__decap_4 PHY_6 ();
 sky130_fd_sc_hd__decap_4 PHY_60 ();
 sky130_fd_sc_hd__decap_4 PHY_61 ();
 sky130_fd_sc_hd__decap_4 PHY_62 ();
 sky130_fd_sc_hd__decap_4 PHY_63 ();
 sky130_fd_sc_hd__decap_4 PHY_64 ();
 sky130_fd_sc_hd__decap_4 PHY_65 ();
 sky130_fd_sc_hd__decap_4 PHY_66 ();
 sky130_fd_sc_hd__decap_4 PHY_67 ();
 sky130_fd_sc_hd__decap_4 PHY_68 ();
 sky130_fd_sc_hd__decap_4 PHY_69 ();
 sky130_fd_sc_hd__decap_4 PHY_7 ();
 sky130_fd_sc_hd__decap_4 PHY_70 ();
 sky130_fd_sc_hd__decap_4 PHY_71 ();
 sky130_fd_sc_hd__decap_4 PHY_72 ();
 sky130_fd_sc_hd__decap_4 PHY_73 ();
 sky130_fd_sc_hd__decap_4 PHY_74 ();
 sky130_fd_sc_hd__decap_4 PHY_75 ();
 sky130_fd_sc_hd__decap_4 PHY_76 ();
 sky130_fd_sc_hd__decap_4 PHY_77 ();
 sky130_fd_sc_hd__decap_4 PHY_78 ();
 sky130_fd_sc_hd__decap_4 PHY_79 ();
 sky130_fd_sc_hd__decap_4 PHY_8 ();
 sky130_fd_sc_hd__decap_4 PHY_80 ();
 sky130_fd_sc_hd__decap_4 PHY_81 ();
 sky130_fd_sc_hd__decap_4 PHY_82 ();
 sky130_fd_sc_hd__decap_4 PHY_83 ();
 sky130_fd_sc_hd__decap_4 PHY_84 ();
 sky130_fd_sc_hd__decap_4 PHY_85 ();
 sky130_fd_sc_hd__decap_4 PHY_86 ();
 sky130_fd_sc_hd__decap_4 PHY_87 ();
 sky130_fd_sc_hd__decap_4 PHY_88 ();
 sky130_fd_sc_hd__decap_4 PHY_89 ();
 sky130_fd_sc_hd__decap_4 PHY_9 ();
 sky130_fd_sc_hd__decap_4 PHY_90 ();
 sky130_fd_sc_hd__decap_4 PHY_91 ();
 sky130_fd_sc_hd__decap_4 PHY_92 ();
 sky130_fd_sc_hd__decap_4 PHY_93 ();
 sky130_fd_sc_hd__decap_4 PHY_94 ();
 sky130_fd_sc_hd__decap_4 PHY_95 ();
 sky130_fd_sc_hd__decap_4 PHY_96 ();
 sky130_fd_sc_hd__decap_4 PHY_97 ();
 sky130_fd_sc_hd__decap_4 PHY_98 ();
 sky130_fd_sc_hd__decap_4 PHY_99 ();
 sky130_fd_sc_hd__o311a_1 _069_ (.A1(_045_),
    .A2(_048_),
    .A3(_052_),
    .B1(_054_),
    .C1(SEL_CONV_TIME[2]),
    .X(_055_));
 sky130_fd_sc_hd__nand3b_1 _071_ (.A_N(SEL_CONV_TIME[1]),
    .B(SEL_CONV_TIME[0]),
    .C(_041_),
    .Y(_057_));
 sky130_fd_sc_hd__or3_1 _072_ (.A(SEL_CONV_TIME[1]),
    .B(SEL_CONV_TIME[0]),
    .C(\temp_analog_1.async_counter_0.div_r[13] ),
    .X(_058_));
 sky130_fd_sc_hd__nand3b_1 _073_ (.A_N(\temp_analog_1.async_counter_0.div_r[16] ),
    .B(SEL_CONV_TIME[0]),
    .C(SEL_CONV_TIME[1]),
    .Y(_059_));
 sky130_fd_sc_hd__o2111a_2 _074_ (.A1(\temp_analog_1.async_counter_0.div_r[15] ),
    .A2(_049_),
    .B1(_058_),
    .C1(_059_),
    .D1(SEL_CONV_TIME[3]),
    .X(_060_));
 sky130_fd_sc_hd__mux4_2 _075_ (.A0(\temp_analog_1.async_counter_0.div_r[5] ),
    .A1(\temp_analog_1.async_counter_0.div_r[7] ),
    .A2(\temp_analog_1.async_counter_0.div_r[6] ),
    .A3(\temp_analog_1.async_counter_0.div_r[8] ),
    .S0(SEL_CONV_TIME[1]),
    .S1(SEL_CONV_TIME[0]),
    .X(_061_));
 sky130_fd_sc_hd__a221oi_4 _076_ (.A1(_057_),
    .A2(_060_),
    .B1(_061_),
    .B2(_045_),
    .C1(SEL_CONV_TIME[2]),
    .Y(_062_));
 sky130_fd_sc_hd__o211a_1 _077_ (.A1(_055_),
    .A2(_062_),
    .B1(\temp_analog_1.async_counter_0.WAKE ),
    .C1(CLK_REF),
    .X(\temp_analog_1.async_counter_0.clk_ref_in ));
 sky130_fd_sc_hd__o211a_1 _078_ (.A1(_055_),
    .A2(_062_),
    .B1(\temp_analog_1.async_counter_0.WAKE_pre ),
    .C1(lc_out),
    .X(\temp_analog_1.async_counter_0.clk_sens_in ));
 sky130_fd_sc_hd__inv_1 _079_ (.A(\temp_analog_1.async_counter_0.div_s[0] ),
    .Y(_005_));
 sky130_fd_sc_hd__nor3_2 _082_ (.A(_005_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[0]));
 sky130_fd_sc_hd__inv_1 _083_ (.A(\temp_analog_1.async_counter_0.div_s[1] ),
    .Y(_016_));
 sky130_fd_sc_hd__nor3_2 _084_ (.A(_016_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[1]));
 sky130_fd_sc_hd__inv_1 _085_ (.A(\temp_analog_1.async_counter_0.div_s[2] ),
    .Y(_021_));
 sky130_fd_sc_hd__nor3_2 _086_ (.A(_021_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[2]));
 sky130_fd_sc_hd__inv_1 _087_ (.A(\temp_analog_1.async_counter_0.div_s[3] ),
    .Y(_022_));
 sky130_fd_sc_hd__nor3_1 _088_ (.A(_022_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[3]));
 sky130_fd_sc_hd__inv_1 _089_ (.A(\temp_analog_1.async_counter_0.div_s[4] ),
    .Y(_023_));
 sky130_fd_sc_hd__nor3_2 _090_ (.A(_023_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[4]));
 sky130_fd_sc_hd__inv_1 _091_ (.A(\temp_analog_1.async_counter_0.div_s[5] ),
    .Y(_024_));
 sky130_fd_sc_hd__nor3_1 _092_ (.A(_024_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[5]));
 sky130_fd_sc_hd__inv_1 _093_ (.A(\temp_analog_1.async_counter_0.div_s[6] ),
    .Y(_025_));
 sky130_fd_sc_hd__nor3_1 _094_ (.A(_025_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[6]));
 sky130_fd_sc_hd__inv_1 _095_ (.A(\temp_analog_1.async_counter_0.div_s[7] ),
    .Y(_026_));
 sky130_fd_sc_hd__nor3_1 _096_ (.A(_026_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[7]));
 sky130_fd_sc_hd__inv_1 _097_ (.A(\temp_analog_1.async_counter_0.div_s[8] ),
    .Y(_027_));
 sky130_fd_sc_hd__nor3_1 _098_ (.A(_027_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[8]));
 sky130_fd_sc_hd__inv_1 _099_ (.A(\temp_analog_1.async_counter_0.div_s[9] ),
    .Y(_028_));
 sky130_fd_sc_hd__nor3_1 _102_ (.A(_028_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[9]));
 sky130_fd_sc_hd__inv_1 _103_ (.A(\temp_analog_1.async_counter_0.div_s[10] ),
    .Y(_006_));
 sky130_fd_sc_hd__nor3_1 _104_ (.A(_006_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[10]));
 sky130_fd_sc_hd__inv_1 _105_ (.A(\temp_analog_1.async_counter_0.div_s[11] ),
    .Y(_007_));
 sky130_fd_sc_hd__nor3_1 _106_ (.A(_007_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[11]));
 sky130_fd_sc_hd__inv_1 _107_ (.A(\temp_analog_1.async_counter_0.div_s[12] ),
    .Y(_008_));
 sky130_fd_sc_hd__nor3_1 _108_ (.A(_008_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[12]));
 sky130_fd_sc_hd__inv_1 _109_ (.A(\temp_analog_1.async_counter_0.div_s[13] ),
    .Y(_009_));
 sky130_fd_sc_hd__nor3_1 _110_ (.A(_009_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[13]));
 sky130_fd_sc_hd__inv_1 _111_ (.A(\temp_analog_1.async_counter_0.div_s[14] ),
    .Y(_010_));
 sky130_fd_sc_hd__nor3_1 _112_ (.A(_010_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[14]));
 sky130_fd_sc_hd__inv_1 _113_ (.A(\temp_analog_1.async_counter_0.div_s[15] ),
    .Y(_011_));
 sky130_fd_sc_hd__nor3_1 _114_ (.A(_011_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[15]));
 sky130_fd_sc_hd__inv_1 _115_ (.A(\temp_analog_1.async_counter_0.div_s[16] ),
    .Y(_012_));
 sky130_fd_sc_hd__nor3_1 _116_ (.A(_012_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[16]));
 sky130_fd_sc_hd__inv_1 _117_ (.A(\temp_analog_1.async_counter_0.div_s[17] ),
    .Y(_013_));
 sky130_fd_sc_hd__nor3_1 _118_ (.A(_013_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[17]));
 sky130_fd_sc_hd__inv_1 _119_ (.A(\temp_analog_1.async_counter_0.div_s[18] ),
    .Y(_014_));
 sky130_fd_sc_hd__nor3_1 _120_ (.A(_014_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[18]));
 sky130_fd_sc_hd__inv_1 _121_ (.A(\temp_analog_1.async_counter_0.div_s[19] ),
    .Y(_015_));
 sky130_fd_sc_hd__nor3_1 _122_ (.A(_015_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[19]));
 sky130_fd_sc_hd__inv_1 _123_ (.A(\temp_analog_1.async_counter_0.div_s[20] ),
    .Y(_017_));
 sky130_fd_sc_hd__nor3_1 _124_ (.A(_017_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[20]));
 sky130_fd_sc_hd__inv_1 _125_ (.A(\temp_analog_1.async_counter_0.div_s[21] ),
    .Y(_018_));
 sky130_fd_sc_hd__nor3_1 _126_ (.A(_018_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[21]));
 sky130_fd_sc_hd__inv_1 _127_ (.A(\temp_analog_1.async_counter_0.div_s[22] ),
    .Y(_019_));
 sky130_fd_sc_hd__nor3_1 _128_ (.A(_019_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[22]));
 sky130_fd_sc_hd__inv_1 _129_ (.A(\temp_analog_1.async_counter_0.div_s[23] ),
    .Y(_020_));
 sky130_fd_sc_hd__nor3_1 _130_ (.A(_020_),
    .B(_055_),
    .C(_062_),
    .Y(DOUT[23]));
 sky130_fd_sc_hd__nor2_1 _131_ (.A(_055_),
    .B(_062_),
    .Y(DONE));
 sky130_fd_sc_hd__inv_1 _132_ (.A(\temp_analog_1.async_counter_0.div_r[20] ),
    .Y(_030_));
 sky130_fd_sc_hd__inv_1 _133_ (.A(\temp_analog_1.async_counter_0.div_r[0] ),
    .Y(_000_));
 sky130_fd_sc_hd__inv_1 _134_ (.A(\temp_analog_1.async_counter_0.div_r[1] ),
    .Y(_001_));
 sky130_fd_sc_hd__inv_1 _135_ (.A(\temp_analog_1.async_counter_0.div_r[2] ),
    .Y(_002_));
 sky130_fd_sc_hd__inv_1 _136_ (.A(\temp_analog_1.async_counter_0.div_r[3] ),
    .Y(_003_));
 sky130_fd_sc_hd__inv_1 _137_ (.A(\temp_analog_1.async_counter_0.div_r[4] ),
    .Y(_004_));
 sky130_fd_sc_hd__or2_2 _138_ (.A(\temp_analog_1.async_counter_0.WAKE ),
    .B(\temp_analog_1.async_counter_0.WAKE_pre ),
    .X(_029_));
 sky130_fd_sc_hd__inv_1 _139_ (.A(\temp_analog_1.async_counter_0.div_r[5] ),
    .Y(_068_));
 sky130_fd_sc_hd__inv_1 _140_ (.A(\temp_analog_1.async_counter_0.div_r[19] ),
    .Y(_036_));
 sky130_fd_sc_hd__inv_1 _141_ (.A(\temp_analog_1.async_counter_0.div_r[18] ),
    .Y(_037_));
 sky130_fd_sc_hd__inv_1 _142_ (.A(\temp_analog_1.async_counter_0.div_r[17] ),
    .Y(_038_));
 sky130_fd_sc_hd__inv_1 _143_ (.A(\temp_analog_1.async_counter_0.div_r[16] ),
    .Y(_039_));
 sky130_fd_sc_hd__inv_1 _144_ (.A(\temp_analog_1.async_counter_0.div_r[15] ),
    .Y(_040_));
 sky130_fd_sc_hd__inv_1 _145_ (.A(\temp_analog_1.async_counter_0.div_r[14] ),
    .Y(_041_));
 sky130_fd_sc_hd__inv_1 _146_ (.A(\temp_analog_1.async_counter_0.div_r[13] ),
    .Y(_042_));
 sky130_fd_sc_hd__inv_1 _147_ (.A(\temp_analog_1.async_counter_0.div_r[12] ),
    .Y(_043_));
 sky130_fd_sc_hd__inv_1 _148_ (.A(\temp_analog_1.async_counter_0.div_r[11] ),
    .Y(_044_));
 sky130_fd_sc_hd__inv_1 _149_ (.A(\temp_analog_1.async_counter_0.div_r[10] ),
    .Y(_031_));
 sky130_fd_sc_hd__inv_1 _150_ (.A(\temp_analog_1.async_counter_0.div_r[9] ),
    .Y(_032_));
 sky130_fd_sc_hd__inv_1 _151_ (.A(\temp_analog_1.async_counter_0.div_r[8] ),
    .Y(_033_));
 sky130_fd_sc_hd__inv_1 _152_ (.A(\temp_analog_1.async_counter_0.div_r[7] ),
    .Y(_034_));
 sky130_fd_sc_hd__inv_1 _153_ (.A(\temp_analog_1.async_counter_0.div_r[6] ),
    .Y(_035_));
 sky130_fd_sc_hd__inv_1 _154_ (.A(SEL_CONV_TIME[3]),
    .Y(_045_));
 sky130_fd_sc_hd__nor3_1 _157_ (.A(SEL_CONV_TIME[1]),
    .B(SEL_CONV_TIME[0]),
    .C(\temp_analog_1.async_counter_0.div_r[17] ),
    .Y(_048_));
 sky130_fd_sc_hd__or2b_1 _158_ (.A(SEL_CONV_TIME[0]),
    .B_N(SEL_CONV_TIME[1]),
    .X(_049_));
 sky130_fd_sc_hd__nand2_1 _159_ (.A(SEL_CONV_TIME[1]),
    .B(SEL_CONV_TIME[0]),
    .Y(_050_));
 sky130_fd_sc_hd__or3b_2 _160_ (.A(SEL_CONV_TIME[1]),
    .B(\temp_analog_1.async_counter_0.div_r[18] ),
    .C_N(SEL_CONV_TIME[0]),
    .X(_051_));
 sky130_fd_sc_hd__o221ai_1 _161_ (.A1(\temp_analog_1.async_counter_0.div_r[19] ),
    .A2(_049_),
    .B1(_050_),
    .B2(\temp_analog_1.async_counter_0.div_r[20] ),
    .C1(_051_),
    .Y(_052_));
 sky130_fd_sc_hd__mux4_1 _162_ (.A0(\temp_analog_1.async_counter_0.div_r[9] ),
    .A1(\temp_analog_1.async_counter_0.div_r[10] ),
    .A2(\temp_analog_1.async_counter_0.div_r[11] ),
    .A3(\temp_analog_1.async_counter_0.div_r[12] ),
    .S0(SEL_CONV_TIME[0]),
    .S1(SEL_CONV_TIME[1]),
    .X(_053_));
 sky130_fd_sc_hd__nand2_1 _163_ (.A(_045_),
    .B(_053_),
    .Y(_054_));
 sky130_fd_sc_hd__conb_1 _164_ (.HI(_067_));
 sky130_fd_sc_hd__dfrtn_1 _165_ (.D(_030_),
    .Q(\temp_analog_1.async_counter_0.div_r[20] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[19] ));
 sky130_fd_sc_hd__dfrtn_1 _166_ (.D(_036_),
    .Q(\temp_analog_1.async_counter_0.div_r[19] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[18] ));
 sky130_fd_sc_hd__dfrtn_1 _167_ (.D(_037_),
    .Q(\temp_analog_1.async_counter_0.div_r[18] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[17] ));
 sky130_fd_sc_hd__dfrtn_1 _168_ (.D(_038_),
    .Q(\temp_analog_1.async_counter_0.div_r[17] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[16] ));
 sky130_fd_sc_hd__dfrtn_1 _169_ (.D(_039_),
    .Q(\temp_analog_1.async_counter_0.div_r[16] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[15] ));
 sky130_fd_sc_hd__dfrtn_1 _170_ (.D(_040_),
    .Q(\temp_analog_1.async_counter_0.div_r[15] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[14] ));
 sky130_fd_sc_hd__dfrtn_1 _171_ (.D(_041_),
    .Q(\temp_analog_1.async_counter_0.div_r[14] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[13] ));
 sky130_fd_sc_hd__dfrtn_1 _172_ (.D(_042_),
    .Q(\temp_analog_1.async_counter_0.div_r[13] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[12] ));
 sky130_fd_sc_hd__dfrtn_1 _173_ (.D(_043_),
    .Q(\temp_analog_1.async_counter_0.div_r[12] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[11] ));
 sky130_fd_sc_hd__dfrtn_1 _174_ (.D(_044_),
    .Q(\temp_analog_1.async_counter_0.div_r[11] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[10] ));
 sky130_fd_sc_hd__dfrtn_1 _175_ (.D(_031_),
    .Q(\temp_analog_1.async_counter_0.div_r[10] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[9] ));
 sky130_fd_sc_hd__dfrtn_1 _176_ (.D(_032_),
    .Q(\temp_analog_1.async_counter_0.div_r[9] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[8] ));
 sky130_fd_sc_hd__dfrtn_1 _177_ (.D(_033_),
    .Q(\temp_analog_1.async_counter_0.div_r[8] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[7] ));
 sky130_fd_sc_hd__dfrtn_1 _178_ (.D(_034_),
    .Q(\temp_analog_1.async_counter_0.div_r[7] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[6] ));
 sky130_fd_sc_hd__dfrtn_1 _179_ (.D(_035_),
    .Q(\temp_analog_1.async_counter_0.div_r[6] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[5] ));
 sky130_fd_sc_hd__dfrtn_1 _180_ (.D(_068_),
    .Q(\temp_analog_1.async_counter_0.div_r[5] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[4] ));
 sky130_fd_sc_hd__dfrtn_1 _181_ (.D(_004_),
    .Q(\temp_analog_1.async_counter_0.div_r[4] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[3] ));
 sky130_fd_sc_hd__dfrtn_1 _182_ (.D(_003_),
    .Q(\temp_analog_1.async_counter_0.div_r[3] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[2] ));
 sky130_fd_sc_hd__dfrtn_1 _183_ (.D(_002_),
    .Q(\temp_analog_1.async_counter_0.div_r[2] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[1] ));
 sky130_fd_sc_hd__dfrtn_1 _184_ (.D(_001_),
    .Q(\temp_analog_1.async_counter_0.div_r[1] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_r[0] ));
 sky130_fd_sc_hd__dfrtn_1 _185_ (.D(_020_),
    .Q(\temp_analog_1.async_counter_0.div_s[23] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[22] ));
 sky130_fd_sc_hd__dfrtn_1 _186_ (.D(_019_),
    .Q(\temp_analog_1.async_counter_0.div_s[22] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[21] ));
 sky130_fd_sc_hd__dfrtn_1 _187_ (.D(_018_),
    .Q(\temp_analog_1.async_counter_0.div_s[21] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[20] ));
 sky130_fd_sc_hd__dfrtn_1 _188_ (.D(_017_),
    .Q(\temp_analog_1.async_counter_0.div_s[20] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[19] ));
 sky130_fd_sc_hd__dfrtn_1 _189_ (.D(_015_),
    .Q(\temp_analog_1.async_counter_0.div_s[19] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[18] ));
 sky130_fd_sc_hd__dfrtn_1 _190_ (.D(_014_),
    .Q(\temp_analog_1.async_counter_0.div_s[18] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[17] ));
 sky130_fd_sc_hd__dfrtn_1 _191_ (.D(_013_),
    .Q(\temp_analog_1.async_counter_0.div_s[17] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[16] ));
 sky130_fd_sc_hd__dfrtn_1 _192_ (.D(_012_),
    .Q(\temp_analog_1.async_counter_0.div_s[16] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[15] ));
 sky130_fd_sc_hd__dfrtn_1 _193_ (.D(_011_),
    .Q(\temp_analog_1.async_counter_0.div_s[15] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[14] ));
 sky130_fd_sc_hd__dfrtn_1 _194_ (.D(_010_),
    .Q(\temp_analog_1.async_counter_0.div_s[14] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[13] ));
 sky130_fd_sc_hd__dfrtn_1 _195_ (.D(_009_),
    .Q(\temp_analog_1.async_counter_0.div_s[13] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[12] ));
 sky130_fd_sc_hd__dfrtn_1 _196_ (.D(_008_),
    .Q(\temp_analog_1.async_counter_0.div_s[12] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[11] ));
 sky130_fd_sc_hd__dfrtn_1 _197_ (.D(_007_),
    .Q(\temp_analog_1.async_counter_0.div_s[11] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[10] ));
 sky130_fd_sc_hd__dfrtn_1 _198_ (.D(_006_),
    .Q(\temp_analog_1.async_counter_0.div_s[10] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[9] ));
 sky130_fd_sc_hd__dfrtn_1 _199_ (.D(_028_),
    .Q(\temp_analog_1.async_counter_0.div_s[9] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[8] ));
 sky130_fd_sc_hd__dfrtn_1 _200_ (.D(_027_),
    .Q(\temp_analog_1.async_counter_0.div_s[8] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[7] ));
 sky130_fd_sc_hd__dfrtn_1 _201_ (.D(_026_),
    .Q(\temp_analog_1.async_counter_0.div_s[7] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[6] ));
 sky130_fd_sc_hd__dfrtn_1 _202_ (.D(_025_),
    .Q(\temp_analog_1.async_counter_0.div_s[6] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[5] ));
 sky130_fd_sc_hd__dfrtn_1 _203_ (.D(_024_),
    .Q(\temp_analog_1.async_counter_0.div_s[5] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[4] ));
 sky130_fd_sc_hd__dfrtn_1 _204_ (.D(_023_),
    .Q(\temp_analog_1.async_counter_0.div_s[4] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[3] ));
 sky130_fd_sc_hd__dfrtn_1 _205_ (.D(_022_),
    .Q(\temp_analog_1.async_counter_0.div_s[3] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[2] ));
 sky130_fd_sc_hd__dfrtn_1 _206_ (.D(_021_),
    .Q(\temp_analog_1.async_counter_0.div_s[2] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[1] ));
 sky130_fd_sc_hd__dfrtn_1 _207_ (.D(_016_),
    .Q(\temp_analog_1.async_counter_0.div_s[1] ),
    .RESET_B(RESET_COUNTERn),
    .CLK_N(\temp_analog_1.async_counter_0.div_s[0] ));
 sky130_fd_sc_hd__dfrtp_1 _208_ (.D(_000_),
    .Q(\temp_analog_1.async_counter_0.div_r[0] ),
    .RESET_B(RESET_COUNTERn),
    .CLK(\temp_analog_1.async_counter_0.clk_ref_in ));
 sky130_fd_sc_hd__dfrtp_1 _209_ (.D(_005_),
    .Q(\temp_analog_1.async_counter_0.div_s[0] ),
    .RESET_B(RESET_COUNTERn),
    .CLK(\temp_analog_1.async_counter_0.clk_sens_in ));
 sky130_fd_sc_hd__dfrtp_1 _210_ (.D(_067_),
    .Q(\temp_analog_1.async_counter_0.WAKE_pre ),
    .RESET_B(RESET_COUNTERn),
    .CLK(CLK_REF));
 sky130_fd_sc_hd__dfrtp_1 _211_ (.D(_029_),
    .Q(\temp_analog_1.async_counter_0.WAKE ),
    .RESET_B(RESET_COUNTERn),
    .CLK(CLK_REF));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_0  (.A(\temp_analog_0.n1 ),
    .Y(\temp_analog_0.n2 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_1  (.A(\temp_analog_0.n2 ),
    .Y(\temp_analog_0.n3 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_2  (.A(\temp_analog_0.n3 ),
    .Y(\temp_analog_0.n4 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_3  (.A(\temp_analog_0.n4 ),
    .Y(\temp_analog_0.n5 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_4  (.A(\temp_analog_0.n5 ),
    .Y(\temp_analog_0.n6 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_5  (.A(\temp_analog_0.n6 ),
    .Y(\temp_analog_0.n7 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_m1  (.A(\temp_analog_0.n7 ),
    .Y(out));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_m2  (.A(\temp_analog_0.n7 ),
    .Y(\temp_analog_0.nx2 ));
 sky130_fd_sc_hd__inv_1 \temp_analog_0.a_inv_m3  (.A(\temp_analog_0.nx2 ),
    .Y(outb));
 sky130_fd_sc_hd__nand2_1 \temp_analog_0.a_nand_0  (.A(en),
    .B(\temp_analog_0.n7 ),
    .Y(\temp_analog_0.n1 ));
 HEADER \temp_analog_1.a_header_0  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_1  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_2  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_3  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_4  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_5  (.VIN(\temp_analog_1.VIN ));
 HEADER \temp_analog_1.a_header_6  (.VIN(\temp_analog_1.VIN ));
 SLC \temp_analog_1.a_lc_0  (.IN(out),
    .INB(outb),
    .VOUT(lc_out));
endmodule
