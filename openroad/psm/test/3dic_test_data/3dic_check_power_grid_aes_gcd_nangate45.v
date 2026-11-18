// This file describes only the power connections between the two chiplets.
module aes (VDD_1, VDD_2, VDD_3, VDD_4, VSS_1, VSS_2);
  inout VDD_1, VDD_2, VDD_3, VDD_4;
  inout VSS_1, VSS_2;
endmodule

module gcd (VDD_1, VDD_2, VDD_3, VDD_4, VSS_1, VSS_2);
  inout VDD_1, VDD_2, VDD_3, VDD_4;
  inout VSS_1, VSS_2;
endmodule

module top (VDD, VSS);
  inout VDD, VSS;

  aes chipA (
    .VDD_1(VDD),
    .VDD_2(VDD),
    .VDD_3(VDD),
    .VDD_4(VDD),
    .VSS_1(VSS),
    .VSS_2(VSS)
  );

  gcd chipB (
    .VDD_1(VDD),
    .VDD_2(VDD),
    .VDD_3(VDD),
    .VDD_4(VDD),
    .VSS_1(VSS),
    .VSS_2(VSS)
  );
endmodule
