// TOP: top
// TECH: nangate45
// TARGETS: escaped_port, positional_connection, depth_2
// CLUE: submodule with all-escaped port names connected POSITIONALLY from
// top; the writer will emit named connections and must escape them right.
module subpos (input \a+a , input \b-b , output \z.z );
  NAND2_X1 g1 (.A1(\a+a ), .A2(\b-b ), .ZN(\z.z ));
endmodule
module top (input a, input b, output z);
  subpos u1 (a, b, z);
endmodule
