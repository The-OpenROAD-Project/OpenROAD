// TOP: top
// TECH: nangate45
// TARGETS: escaped_module, char_slash, depth_2
// CLUE: module named \m/s ; slash is OpenROAD's flatten separator, a
// module NAME carrying it probes the writer's escaping of module names.
module \m/s (input a, output z);
  INV_X1 g1 (.A(a), .ZN(z));
endmodule
module top (input a, output z);
  \m/s u1 (.a(a), .z(z));
endmodule
