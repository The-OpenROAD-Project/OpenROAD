// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string_view>
#include <utility>
#include <vector>

namespace syn {

// Each entry is {systemverilog_source, expected_syn_ir_text}. The IR side
// is the post-normalize dump (syn::Graph::dump format). When it's empty,
// elab_test / verific_test will run the frontend and dump the gate-side
// IR on failure so it can be pasted in as the golden.
inline const std::vector<std::pair<std::string_view, std::string_view>>
    elabTestcases = {
        // =======================================================
        // Binary bitwise
        // =======================================================
        {
            R"(
module and4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a & b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:4
%15:4 = and %6:4 %10:4
)",
        },
        {
            R"(
module or4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a | b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:4
%15:4 = or %6:4 %10:4
)",
        },
        {
            R"(
module xor4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a ^ b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:4
%15:4 = xor %6:4 %10:4
)",
        },
        {
            R"(
module xnor4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a ~^ b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %19:4
%15:4 = xor %6:4 %10:4
%19:4 = not %15:4
)",
        },
        {
            R"(
module nand4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = ~(a & b);
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %19:4
%15:4 = and %6:4 %10:4
%19:4 = not %15:4
)",
        },
        {
            R"(
module nor4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = ~(a | b);
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %19:4
%15:4 = or %6:4 %10:4
%19:4 = not %15:4
)",
        },

        // =======================================================
        // Binary arithmetic
        // =======================================================
        {
            R"(
module adder4(input [3:0] a, input [2:0] b, output [4:0] y);
  assign y = a + b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:3 = input "b"
%14:0 = output "y" %15:5
%15:6 = adc [ 0 %6+0:4 ] [ 00 %10+0:3 ] 0
)",
        },
        {
            R"(
module sub4(input [3:0] a, input [3:0] b, output [4:0] y);
  assign y = a - b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %20:5
%15:5 = not [ 0 %10+0:4 ]
%20:6 = adc [ 0 %6+0:4 ] %15:5 1
)",
        },
        {
            R"(
module mul4(input [3:0] a, input [3:0] b, output [7:0] y);
  assign y = a * b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:8
%15:8 = mul [ 0000 %6+0:4 ] [ 0000 %10+0:4 ]
)",
        },
        {
            R"(
module udiv4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a / b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:4
%15:4 = udiv %6:4 %10:4
)",
        },
        {
            R"(
module umod4(input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = a % b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15:4
%15:4 = umod %6:4 %10:4
)",
        },

        // =======================================================
        // Binary relational
        // =======================================================
        {
            R"(
module ult4(input [3:0] a, input [3:0] b, output y);
  assign y = a < b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15
%15:1 = ult %6:4 %10:4
)",
        },
        {
            R"(
module ule4(input [3:0] a, input [3:0] b, output y);
  assign y = a <= b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %16
%15:1 = ult %10:4 %6:4
%16:1 = not %15
)",
        },
        {
            R"(
module ugt4(input [3:0] a, input [3:0] b, output y);
  assign y = a > b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15
%15:1 = ult %10:4 %6:4
)",
        },
        {
            R"(
module uge4(input [3:0] a, input [3:0] b, output y);
  assign y = a >= b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %16
%15:1 = ult %6:4 %10:4
%16:1 = not %15
)",
        },
        {
            R"(
module slt4(input signed [3:0] a, input signed [3:0] b, output y);
  assign y = a < b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15
%15:1 = slt %6:4 %10:4
)",
        },

        // =======================================================
        // Binary equality
        // =======================================================
        {
            R"(
module eq4(input [3:0] a, input [3:0] b, output y);
  assign y = a == b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %15
%15:1 = eq %6:4 %10:4
)",
        },
        {
            R"(
module neq4(input [3:0] a, input [3:0] b, output y);
  assign y = a != b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %16
%15:1 = eq %6:4 %10:4
%16:1 = not %15
)",
        },

        // =======================================================
        // Binary logical
        // =======================================================
        {
            R"(
module logand4(input [3:0] a, input [3:0] b, output y);
  assign y = a && b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %21
%15:1 = or %6 %6+1:1
%16:1 = or %15 %6+2:1
%17:1 = or %16 %6+3:1
%18:1 = or %10 %10+1:1
%19:1 = or %18 %10+2:1
%20:1 = or %19 %10+3:1
%21:1 = and %17 %20
)",
        },
        {
            R"(
module logor4(input [3:0] a, input [3:0] b, output y);
  assign y = a || b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:4 = input "b"
%14:0 = output "y" %21
%15:1 = or %6 %6+1:1
%16:1 = or %15 %6+2:1
%17:1 = or %16 %6+3:1
%18:1 = or %10 %10+1:1
%19:1 = or %18 %10+2:1
%20:1 = or %19 %10+3:1
%21:1 = or %17 %20
)",
        },

        // =======================================================
        // Binary shift
        // =======================================================
        {
            R"(
module shl4(input [3:0] a, input [1:0] b, output [3:0] y);
  assign y = a << b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:2 = input "b"
%12:0 = output "y" %13:4
%13:4 = shl %6:4 %10:2 #1
)",
        },
        {
            R"(
module ushr4(input [3:0] a, input [1:0] b, output [3:0] y);
  assign y = a >> b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:2 = input "b"
%12:0 = output "y" %13:4
%13:4 = ushr %6:4 %10:2 #1
)",
        },
        {
            R"(
module sshl4(input signed [3:0] a, input [1:0] b, output signed [3:0] y);
  assign y = a <<< b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:2 = input "b"
%12:0 = output "y" %13:4
%13:4 = shl %6:4 %10:2 #1
)",
        },
        {
            R"(
module sshr4(input signed [3:0] a, input [1:0] b, output signed [3:0] y);
  assign y = a >>> b;
endmodule
)",
            R"(
%6:4 = input "a"
%10:2 = input "b"
%12:0 = output "y" %13:4
%13:4 = sshr %6:4 %10:2 #1
)",
        },

        // =======================================================
        // Unary arithmetic / bitwise / logical
        // =======================================================
        {
            R"(
module uplus4(input [3:0] a, output [3:0] y);
  assign y = +a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %5:4
)",
        },
        {
            R"(
module uminus4(input [3:0] a, output [3:0] y);
  assign y = -a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %14:4
%10:4 = not %5:4
%14:5 = adc %10:4 0001 0
)",
        },
        {
            R"(
module unot4(input [3:0] a, output [3:0] y);
  assign y = ~a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %10:4
%10:4 = not %5:4
)",
        },
        {
            R"(
module lognot4(input [3:0] a, output y);
  assign y = !a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %13
%10:1 = or %5 %5+1:1
%11:1 = or %10 %5+2:1
%12:1 = or %11 %5+3:1
%13:1 = not %12
)",
        },

        // =======================================================
        // Unary reduction (all produce 1-bit output)
        // =======================================================
        {
            R"(
module redand4(input [3:0] a, output y);
  assign y = &a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %12
%10:1 = and %5 %5+1:1
%11:1 = and %10 %5+2:1
%12:1 = and %11 %5+3:1
)",
        },
        {
            R"(
module rednand4(input [3:0] a, output y);
  assign y = ~&a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %13
%10:1 = and %5 %5+1:1
%11:1 = and %10 %5+2:1
%12:1 = and %11 %5+3:1
%13:1 = not %12
)",
        },
        {
            R"(
module redor4(input [3:0] a, output y);
  assign y = |a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %12
%10:1 = or %5 %5+1:1
%11:1 = or %10 %5+2:1
%12:1 = or %11 %5+3:1
)",
        },
        {
            R"(
module rednor4(input [3:0] a, output y);
  assign y = ~|a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %13
%10:1 = or %5 %5+1:1
%11:1 = or %10 %5+2:1
%12:1 = or %11 %5+3:1
%13:1 = not %12
)",
        },
        {
            R"(
module redxor4(input [3:0] a, output y);
  assign y = ^a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %12
%10:1 = xor %5 %5+1:1
%11:1 = xor %10 %5+2:1
%12:1 = xor %11 %5+3:1
)",
        },
        {
            R"(
module redxnor4(input [3:0] a, output y);
  assign y = ~^a;
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "y" %13
%10:1 = xor %5 %5+1:1
%11:1 = xor %10 %5+2:1
%12:1 = xor %11 %5+3:1
%13:1 = not %12
)",
        },
        {
            R"(
module mux4(input s, input [3:0] a, input [3:0] b, output [3:0] y);
  assign y = s ? a : b;
endmodule
)",
            R"(
%7:1 = input "s"
%8:4 = input "a"
%12:4 = input "b"
%16:0 = output "y" %17:4
%17:4 = mux %7 %8:4 %12:4
)",
        },
        {
            R"(
module priority8 #(
  parameter WIDTH = 8
) (input [WIDTH-1:0] bits, output logic [$clog2(WIDTH)-1:0] encoded);
	always_comb begin: for_loop_priority_encoder
		automatic integer i = 0;
		for (i = 0; i < WIDTH; i++) begin
			if (bits[i]) begin
				break;
			end
		end
		encoded = i;
	end
endmodule
)",
            R"(
%3:0 = name "bits" 0 8 vector %5:8
%4:0 = name "encoded" 0 3 vector %86:3
%5:8 = input "bits"
%13:0 = output "encoded" %86:3
%14:2 = ushr 10 %5 #1
%16:2 = ushr [ 1 %14 ] %5+1:1 #1
%18:2 = ushr [ 1 %16 ] %5+2:1 #1
%20:2 = ushr [ 1 %18 ] %5+3:1 #1
%22:2 = ushr [ 1 %20 ] %5+4:1 #1
%24:2 = ushr [ 1 %22 ] %5+5:1 #1
%26:2 = ushr [ 1 %24 ] %5+6:1 #1
%28:2 = ushr [ 1 %26 ] %5+7:1 #1
%30:8 = ushr 01111000 %28 #4
%38:8 = ushr [ 0110 %30+0:4 ] %26 #4
%46:8 = ushr [ 0101 %38+0:4 ] %24 #4
%54:8 = ushr [ 0100 %46+0:4 ] %22 #4
%62:8 = ushr [ 0011 %54+0:4 ] %20 #4
%70:8 = ushr [ 0010 %62+0:4 ] %18 #4
%78:8 = ushr [ 0001 %70+0:4 ] %16 #4
%86:8 = ushr [ 0000 %78+0:4 ] %14 #4
)",
        },
        {
            R"(
module sel8 (input [7:0] data, input [3:0] sel, output [1:0] y);
	assign y = data[sel+:2];
endmodule
)",
            R"(
%3:0 = name "data" 0 8 vector %6:8
%4:0 = name "sel" 0 4 vector %14:4
%5:0 = name "y" 0 2 vector %19:2
%6:8 = input "data"
%14:4 = input "sel"
%18:0 = output "y" %19:2
%19:8 = xshr %6:8 [ 0 %14+0:4 ] #1
)",
        },
        {
            R"(
module wand_wor (input logic [3:0] a, input logic [3:0] b,
                 output wor [3:0] wor_net, output wand [3:0] wand_net);
  wire [1:0] other_wire;
  assign {wor_net[3:2], other_wire[1:0]} = a;
  assign wor_net = b;
  assign wand_net[1:0] = other_wire;
  assign wand_net = b;
  assign {wor_net[3], wand_net[3]} = 2;
endmodule
)",
            R"(
%3:0 = name "a" 0 4 vector %8:4
%4:0 = name "b" 0 4 vector %12:4
%5:0 = name "wor_net" 0 4 vector [ %20 %18 %12+0:2 ]
%6:0 = name "wand_net" 0 4 vector [ %23 %12+2 %22 %21 ]
%7:0 = name "other_wire" 0 2 vector %8:2
%8:4 = input "a"
%12:4 = input "b"
%16:0 = output "wor_net" [ %20 %18 %12+0:2 ]
%17:0 = output "wand_net" [ %23 %12+2 %22 %21 ]
%18:1 = or %8+2:1 %12+2:1
%19:1 = or %8+3:1 %12+3:1
%20:1 = or %19 1
%21:1 = and %8 %12
%22:1 = and %8+1:1 %12+1:1
%23:1 = and %12+3:1 0
)",
        },
        {
            R"(
module foreach_simple (input logic [3:0] a, output logic [3:0] b);
  always_comb begin
    foreach (a[i]) b[i] = a[i];
  end
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "b" %5:4
)",
        },
        {
            // slang-elab issue 110
            R"(
module func_logic_arg (input logic [7:0] a, input logic [7:0] b,
                       output logic [8:0] y);
  function [8:0] my_sum;
    input [7:0] a;
    input [7:0] b;
    begin
      my_sum = a + b;
    end
  endfunction
  assign y = my_sum(a, b);
endmodule
)",
            R"(
%8:8 = input "a"
%16:8 = input "b"
%24:0 = output "y" %25:9
%25:10 = adc [ 0 %8+0:8 ] [ 0 %16+0:8 ] 0
)",
        },
        {
            // slang-elab issue 174
            R"(
module func_recurse (input logic [3:0] inp, output logic [3:0] out);
  function automatic [3:0] pow_a;
    input [3:0] base;
    input [3:0] exp;
    begin
      pow_a = 1;
      if (exp > 0)
        pow_a = base * pow_a(base, exp - 1);
    end
  endfunction
  assign out = pow_a(inp, 3);
endmodule
)",
            R"(
%7:4 = input "inp"
%11:0 = output "out" %20:4
%12:4 = mul %7:4 0001
%16:4 = mul %7:4 %12:4
%20:4 = mul %7:4 %16:4
)",
        },
        {
            // slang-elab issue 176
            R"(
module func_recurse_mutual (input logic [3:0] inp, output logic [3:0] out);
  function automatic [3:0] flip;
    input [3:0] inp;
    flip = ~inp;
  endfunction
  function automatic [3:0] pow_flip_b;
    input [3:0] base;
    input [3:0] exp;
    begin
      pow_flip_b = 1;
      if (exp > 0)
        pow_flip_b = base * pow_flip_b(flip(base), exp - 1);
    end
  endfunction
  assign out = pow_flip_b(2, 2) ^ inp;
endmodule
)",
            R"(
%3:0 = name "inp" 0 4 vector %8:4
%4:0 = name "out" 0 4 vector %13:4
%8:4 = input "inp"
%12:0 = output "out" %13:4
%13:4 = xor 1010 %8:4
)",
        },
        {
            // slang-elab issue 78
            R"(
module width_cast_shl
  #(parameter int unsigned W = 8)
  (input  logic [$clog2(W)-1:0] s,
   output logic [W-1:0] y);
  assign y = (W)'(1'b1 << s);
endmodule
)",
            R"(
%3:0 = name "s" 0 3 vector %5:3
%4:0 = name "y" 0 8 vector %9:8
%5:3 = input "s"
%8:0 = output "y" %9:8
%9:8 = shl 00000001 %5:3 #1
)",
        },
        {
            // slang-elab issue 142
            R"(
module init_rom (input logic [3:0] addr, output logic [7:0] data);
  reg [7:0] rom [0:15];
  initial begin
    rom[0]  = 8'h48;
    rom[1]  = 8'h65;
    rom[2]  = 8'h6c;
    rom[3]  = 8'h6c;
    rom[4]  = 8'h6f;
    rom[5]  = 8'h2c;
    rom[6]  = 8'h20;
    rom[7]  = 8'h77;
    rom[8]  = 8'h6f;
    rom[9]  = 8'h72;
    rom[10] = 8'h6c;
    rom[11] = 8'h64;
    rom[12] = 8'h21;
    rom[13] = 8'h00;
    rom[14] = 8'h00;
    rom[15] = 8'h00;
  end
  always_comb data = rom[addr];
endmodule
)",
            R"(
%3:0 = name "addr" 0 4 vector %6:4
%4:0 = name "data" 0 8 vector %154:8
%6:4 = input "addr"
%10:0 = output "data" %154:8
%11:5 = not [ 0 %6+0:4 ]
%16:1 = slt %11:5 10000
%17:1 = not %16
%18:128 = ushr 01001000011001010110110001101100011011110010110000100000011101110110111101110010011011000110010000100001000000000000000000000000 %11:4 #8
%146:8 = mux %17 %18:8 XXXXXXXX
%154:8 = mux %11+4 %146:8 XXXXXXXX
)",
        },
        {
            // slang-elab issue 161
            R"(
module postdec_comb (input  logic [7:0] a, input  logic [7:0] b,
                     output logic [7:0] diff, output logic [7:0] var_out);
  always_comb begin
    automatic logic [7:0] v = a;
    diff    = (v--) - b;
    var_out = v;
  end
endmodule
)",
            R"(
%7:8 = input "a"
%15:8 = input "b"
%23:0 = output "diff" %33:8
%24:0 = output "var_out" %50:8
%25:8 = not %15:8
%33:9 = adc %7:8 %25:8 1
%42:8 = not 00000001
%50:9 = adc %7:8 %42:8 1
)",
        },
        {
            R"(
module bitstream_rev (input logic [7:0] a, output logic [7:0] y);
  assign y = {<<1{a}};
endmodule
)",
            R"(
%5:8 = input "a"
%13:0 = output "y" [ %5 %5+1 %5+2 %5+3 %5+4 %5+5 %5+6 %5+7 ]
)",
        },
        {
            R"(
module bitstream_chunk_rev (input logic [7:0] a, output logic [7:0] y);
  assign y = {<<4{a}};
endmodule
)",
            R"(
%5:8 = input "a"
%13:0 = output "y" [ %5+0:4 %5+4:4 ]
)",
        },
        {
            R"(
module wildcard_eq (input logic [3:0] a, output logic match);
  assign match = (a ==? 4'b1?0?);
endmodule
)",
            R"(
%5:4 = input "a"
%9:0 = output "match" %10
%10:1 = eq [ %5+3 %5+1 ] 10
)",
        },
        {
            R"(
module struct_cast_reorder (input logic [4:0] data, output logic [4:0] y);
  typedef struct {
    logic       a;
    logic [1:0] b;
    logic       c;
    logic       d;
  } unpacked_t;
  unpacked_t x;
  always_comb x = unpacked_t'(data);
  assign y = {x.a, x.c, x.d, x.b};
endmodule
)",
            R"(
%6:5 = input "data"
%11:0 = output "y" [ %6+4 %6+0:2 %6+2:2 ]
)",
        },
        {
            R"(
module pre_post_incdec (input  logic [7:0]  v,
                        output logic [15:0] decrpre_y,
                        output logic [15:0] decrpost_y,
                        output logic [15:0] incrpre_y,
                        output logic [15:0] incrpost_y);
  function automatic [15:0] decrpre(logic [7:0] v);
    logic [7:0] a, b;
    a = v--;
    b = v;
    return {a, b};
  endfunction
  function automatic [15:0] decrpost(logic [7:0] v);
    logic [7:0] a, b;
    a = --v;
    b = v;
    return {a, b};
  endfunction
  function automatic [15:0] incrpre(logic [7:0] v);
    logic [7:0] a, b;
    a = v++;
    b = v;
    return {a, b};
  endfunction
  function automatic [15:0] incrpost(logic [7:0] v);
    logic [7:0] a, b;
    a = ++v;
    b = v;
    return {a, b};
  endfunction
  assign decrpre_y  = decrpre(v);
  assign decrpost_y = decrpost(v);
  assign incrpre_y  = incrpre(v);
  assign incrpost_y = incrpost(v);
endmodule
)",
            R"(
%12:8 = input "v"
%20:0 = output "decrpre_y" [ %12+0:8 %32+0:8 ]
%21:0 = output "decrpost_y" [ %49+0:8 %49+0:8 ]
%22:0 = output "incrpre_y" [ %12+0:8 %58+0:8 ]
%23:0 = output "incrpost_y" [ %67+0:8 %67+0:8 ]
%24:8 = not 00000001
%32:9 = adc %12:8 %24:8 1
%41:8 = not 00000001
%49:9 = adc %12:8 %41:8 1
%58:9 = adc %12:8 00000001 0
%67:9 = adc %12:8 00000001 0
)",
        },
        {
            R"(
module unpacked_destructure (input  logic [4:0] e0, input logic [4:0] e1,
                             output logic [9:0] y);
  logic [4:0] data[2];
  logic [4:0] a, b;
  always_comb begin
    data[0] = e0;
    data[1] = e1;
    '{a, b} = data;
  end
  assign y = {a, b};
endmodule
)",
            R"(
%9:5 = input "e0"
%14:5 = input "e1"
%19:0 = output "y" [ %9+0:5 %14+0:5 ]
)",
        },
        {
            R"(
module tied(output wire a);
  assign a = 1;
endmodule
)",
            R"(
%3:0 = output "a" 1
)",
        },
        {
            R"(
module initialized(output logic a);
  initial a = 1;
endmodule
)",
            R"(
%3:0 = output "a" 1
)",
        },
        {
            R"(
module uart_tx_ctrl (
  input  logic [2:0] state,
  input  logic       start,
  input  logic       baud_tick,
  input  logic [2:0] bit_idx,
  output logic [2:0] next_state,
  output logic       tx_busy,
  output logic       load_shift,
  output logic       inc_bit
);
  localparam IDLE  = 3'd0;
  localparam START = 3'd1;
  localparam DATA  = 3'd2;
  localparam STOP  = 3'd3;
  localparam DONE  = 3'd4;

  always_comb begin
    next_state = state;
    tx_busy    = 1'b1;
    load_shift = 1'b0;
    inc_bit    = 1'b0;

    case (state)
      IDLE: begin
        tx_busy = 1'b0;
        if (start) begin
          next_state = START;
          load_shift = 1'b1;
        end
      end
      START: begin
        if (baud_tick) next_state = DATA;
      end
      DATA: begin
        if (baud_tick) begin
          if (bit_idx == 3'd7)
            next_state = STOP;
          else
            inc_bit = 1'b1;
        end
      end
      STOP: begin
        if (baud_tick) next_state = DONE;
      end
      DONE: begin
        next_state = IDLE;
        tx_busy    = 1'b0;
      end
      default: next_state = IDLE;
    endcase
  end
endmodule
)",
            R"(
%3:0 = name "state" 0 3 vector %11:3
%4:0 = name "start" 0 1 %14
%5:0 = name "baud_tick" 0 1 %15
%6:0 = name "bit_idx" 0 3 vector %16:3
%7:0 = name "next_state" 0 3 vector %54:3
%8:0 = name "tx_busy" 0 1 %78
%9:0 = name "load_shift" 0 1 %88
%10:0 = name "inc_bit" 0 1 %100
%11:3 = input "state"
%14:1 = input "start"
%15:1 = input "baud_tick"
%16:3 = input "bit_idx"
%19:0 = output "next_state" %54:3
%20:0 = output "tx_busy" %78
%21:0 = output "load_shift" %88
%22:0 = output "inc_bit" %100
%23:6 = ushr [ 001 %11+0:3 ] %14 #3
%29:6 = ushr [ 010 %11+0:3 ] %15 #3
%35:1 = eq %16:3 111
%36:6 = ushr [ 011 %11+0:3 ] %35 #3
%42:6 = ushr [ %36+0:3 %11+0:3 ] %15 #3
%48:6 = ushr [ 100 %11+0:3 ] %15 #3
%54:24 = ushr [ 000000000000 %48+0:3 %42+0:3 %29+0:3 %23+0:3 ] %11:3 #3
%78:8 = ushr 11101110 %11:3 #1
%86:2 = ushr 10 %14 #1
%88:8 = ushr [ 0000000 %86 ] %11:3 #1
%96:2 = ushr 01 %35 #1
%98:2 = ushr [ %96 0 ] %15 #1
%100:8 = ushr [ 00000 %98 00 ] %11:3 #1
)",
        },
        {
            // Mimics core/decoder.sv:191 — a wide (7-bit) opcode selector
            // with sparse, mutually-exclusive branches against constant
            // matches,
            // and a nested unique case inside one branch. This is the exact
            // shape that produced Select_N in /tmp/cva6.ir.
            R"(
module instr_decode (
  input  logic [6:0] opcode,
  input  logic [2:0] funct3,
  output logic [3:0] fu,
  output logic [4:0] op
);
  always_comb begin
    fu = '0;
    op = '0;
    case (opcode)
      7'b0110011: begin
        fu = 4'd1;
        unique case (funct3)
          3'd0: op = 5'd1;
          3'd1: op = 5'd2;
          3'd2: op = 5'd3;
          3'd4: op = 5'd11;
          default: op = 5'd0;
        endcase
      end
      7'b0010011: begin fu = 4'd1; op = 5'd4;  end
      7'b0000011: begin fu = 4'd2; op = 5'd5;  end
      7'b0100011: begin fu = 4'd2; op = 5'd6;  end
      7'b1100011: begin fu = 4'd3; op = 5'd7;  end
      7'b1101111: begin fu = 4'd3; op = 5'd8;  end
      7'b1100111: begin fu = 4'd3; op = 5'd9;  end
      7'b0110111: begin fu = 4'd1; op = 5'd10; end
      default:    begin fu = '0;   op = '0;   end
    endcase
  end
endmodule
)",
            R"(
%7:7 = input "opcode"
%14:3 = input "funct3"
%17:0 = output "fu" [ 00 %41+0:2 ]
%18:0 = output "op" [ 0 %98 %87+0:3 ]
%19:1 = eq %7:7 0110011
%20:1 = eq %7:7 0010011
%21:1 = eq %7:7 0000011
%22:1 = eq %7:7 0100011
%23:1 = eq %7:7 1100011
%24:1 = eq %7:7 1101111
%25:1 = eq %7:7 1100111
%26:1 = eq %7:7 0110111
%27:2 = mux %26 01 00
%29:2 = mux %25 11 %27:2
%31:2 = mux %24 11 %29:2
%33:2 = mux %23 11 %31:2
%35:2 = mux %22 10 %33:2
%37:2 = mux %21 10 %35:2
%39:2 = mux %20 01 %37:2
%41:2 = mux %19 01 %39:2
%43:16 = ushr 0000001100111001 %14:3 #2
%59:4 = mux %26 1010 0000
%63:4 = mux %25 1001 %59:4
%67:4 = mux %24 1000 %63:4
%71:4 = mux %23 0111 %67:4
%75:4 = mux %22 0110 %71:4
%79:4 = mux %21 0101 %75:4
%83:4 = mux %20 0100 %79:4
%87:3 = mux %19 [ 0 %43+0:2 ] %83:3
%90:8 = ushr 00010000 %14:3 #1
%98:1 = mux %19 %90 %83+3:1
)",
        },
        {
            R"(
module seltest_unique (
  input  logic [7:0] opt1,
  input  logic [7:0] opt2,
  input  logic [7:0] opt3,
  input  logic [7:0] opt4,
  input  logic a,
  input  logic b,
  input  logic c,
  input  logic d,
  output logic [7:0] out
);
  always_comb begin
    out = '0;
    unique case (1'b1)
      a: out = opt1;
      b: out = opt2;
      c: out = opt3;
      d: out = opt4;
      default: out = '0;
    endcase
  end
endmodule
)",
            R"(
%12:8 = input "opt1"
%20:8 = input "opt2"
%28:8 = input "opt3"
%36:8 = input "opt4"
%44:1 = input "a"
%45:1 = input "b"
%46:1 = input "c"
%47:1 = input "d"
%48:0 = output "out" %77:8
%49:1 = eq 1 %44
%50:1 = eq 1 %45
%51:1 = eq 1 %46
%52:1 = eq 1 %47
%53:8 = mux %52 %36:8 00000000
%61:8 = mux %51 %28:8 %53:8
%69:8 = mux %50 %20:8 %61:8
%77:8 = mux %49 %12:8 %69:8
)",
        },
        {
            R"(
module seltest (
  input  logic [7:0] opt1,
  input  logic [7:0] opt2,
  input  logic [7:0] opt3,
  input  logic [7:0] opt4,
  input  logic a,
  input  logic b,
  input  logic c,
  input  logic d,
  output logic [7:0] out
);
  always_comb begin
    out = 0;
    if (a)
        out = opt1;
    else if (b)
        out = opt2;
    else if (c)
        out = opt3;
    else if (d)
        out = opt4;
  end
endmodule
)",
            R"(
%12:8 = input "opt1"
%20:8 = input "opt2"
%28:8 = input "opt3"
%36:8 = input "opt4"
%44:1 = input "a"
%45:1 = input "b"
%46:1 = input "c"
%47:1 = input "d"
%48:0 = output "out" %97:8
%49:16 = ushr [ %36+0:8 00000000 ] %47 #8
%65:16 = ushr [ %28+0:8 %49+0:8 ] %46 #8
%81:16 = ushr [ %20+0:8 %65+0:8 ] %45 #8
%97:16 = ushr [ %12+0:8 %81+0:8 ] %44 #8
)",
        },
};

}  // namespace syn
