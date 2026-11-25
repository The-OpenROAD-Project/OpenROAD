// Enable registers (sky130hd edfxtp) inside a module of a hierarchical
// netlist.  In `core`, r1 and r2 share the enable `en` and feed one NAND:
// they are one class and merge into a single edfxtp after the gate.  r4
// (enable `en2`) and r6 (enable `en`) feed g10: a mixed-class gate, so both
// are unmapped to plain registers before they merge, and the cells that
// replace them are created inside `core`, on the module's own enable and
// clock ports.  Delay buffers after g10 make the path after the gate longer
// than the hold loop of an unmapped register, so the first slide pays.
// rst_n is a synchronous reset in logic and the enables are
// forced on while it is asserted, so every register settles to a defined
// value for the sequential equivalence check.
module core (a, b, c, d, en, en2, rn, ck, y, z);
  input a;
  input b;
  input c;
  input d;
  input en;
  input en2;
  input rn;
  input ck;
  output y;
  output z;
  wire d1, d2, d4, d6, q1, q2, q4, q6;
  wire n1, n2, n3, n4, n5, n6, d3, q3, m1, m2, m3, m4, m5, m6, d5, q5;

  sky130_fd_sc_hd__and2_1 g0 (.A(a), .B(rn), .X(d1));
  sky130_fd_sc_hd__and2_1 g1 (.A(b), .B(rn), .X(d2));
  sky130_fd_sc_hd__and2_1 g1b (.A(c), .B(rn), .X(d4));
  sky130_fd_sc_hd__and2_1 g1c (.A(d), .B(rn), .X(d6));
  sky130_fd_sc_hd__edfxtp_1 r1 (.D(d1), .DE(en), .CLK(ck), .Q(q1));
  sky130_fd_sc_hd__edfxtp_1 r2 (.D(d2), .DE(en), .CLK(ck), .Q(q2));
  sky130_fd_sc_hd__edfxtp_1 r4 (.D(d4), .DE(en2), .CLK(ck), .Q(q4));
  sky130_fd_sc_hd__edfxtp_1 r6 (.D(d6), .DE(en), .CLK(ck), .Q(q6));

  // Same-class cone: r1, r2 -> NAND -> inverters and delay buffers -> r3.
  sky130_fd_sc_hd__nand2_1 g2 (.A(q1), .B(q2), .Y(n1));
  sky130_fd_sc_hd__inv_1 g3 (.A(n1), .Y(n2));
  sky130_fd_sc_hd__dlygate4sd1_1 g4 (.A(n2), .X(n3));
  sky130_fd_sc_hd__dlygate4sd1_1 g5 (.A(n3), .X(n4));
  sky130_fd_sc_hd__inv_1 g6 (.A(n4), .Y(n5));
  sky130_fd_sc_hd__inv_1 g7 (.A(n5), .Y(n6));
  sky130_fd_sc_hd__and2_1 g8 (.A(n6), .B(rn), .X(d3));
  sky130_fd_sc_hd__dfxtp_1 r3 (.D(d3), .CLK(ck), .Q(q3));
  sky130_fd_sc_hd__buf_1 g9 (.A(q3), .X(y));

  // Mixed-class cone: r4 (en2) and r6 (en) -> NOR -> inverters and delay
  // buffers -> r5.
  sky130_fd_sc_hd__nor2_1 g10 (.A(q4), .B(q6), .Y(m1));
  sky130_fd_sc_hd__inv_1 g11 (.A(m1), .Y(m2));
  sky130_fd_sc_hd__dlygate4sd1_1 g12 (.A(m2), .X(m3));
  sky130_fd_sc_hd__dlygate4sd1_1 g13 (.A(m3), .X(m4));
  sky130_fd_sc_hd__inv_1 g14 (.A(m4), .Y(m5));
  sky130_fd_sc_hd__inv_1 g15 (.A(m5), .Y(m6));
  sky130_fd_sc_hd__and2_1 g16 (.A(m6), .B(rn), .X(d5));
  sky130_fd_sc_hd__dfxtp_1 r5 (.D(d5), .CLK(ck), .Q(q5));
  sky130_fd_sc_hd__buf_1 g17 (.A(q5), .X(z));
endmodule

module top (a, b, c, d, en_i, en2_i, rst_n, clk, y, z);
  input a;
  input b;
  input c;
  input d;
  input en_i;
  input en2_i;
  input rst_n;
  input clk;
  output y;
  output z;
  wire rst, en, en2;

  sky130_fd_sc_hd__inv_1 g_r (.A(rst_n), .Y(rst));
  sky130_fd_sc_hd__or2_1 g_e1 (.A(en_i), .B(rst), .X(en));
  sky130_fd_sc_hd__or2_1 g_e2 (.A(en2_i), .B(rst), .X(en2));
  core u_core (.a(a), .b(b), .c(c), .d(d), .en(en), .en2(en2), .rn(rst_n),
               .ck(clk), .y(y), .z(z));
endmodule
