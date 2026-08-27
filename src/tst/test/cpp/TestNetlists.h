// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

// Netlists for the src/tst tests, written to the test's temp dir rather than
// checked in as .v files: OpenROAD's security pre-commit hook blocks *.v
// outside an allowlist of test directories, and src/tst/test is not on it (no
// .v file had ever lived there). They are small enough that inlining costs
// nothing, and it buys something -- the two defective variants are DERIVED from
// the good netlist by a named edit, so the single difference each one is
// testing is visible in the code instead of being a comment on a copy that can
// drift.

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace tst {

// Three submodules, each with a DFF, so the two link modes produce visibly
// different Verilog: nested modules against one module with escaped
// hierarchical instance names.
inline const char* kHierNetlist = R"(module top (in, clk1, clk2, out, out2);
   input in, clk1, clk2;
   output out, out2;

   block1 b1 (.in(in), .clk(clk1), .out(b1out), .out2(out2));
   block2 b2 (.in(b1out), .clk(clk2), .out(out));
endmodule

module block1 (in, clk, out, out2);
   input in, clk;
   output out, out2;

   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
   BUF_X1 u3 (.A(out), .Z(out2));
endmodule

module block2 (in, clk, out);
   input in, clk;
   output out;

   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
endmodule
)";

// Replaces the first occurrence, and throws when there is none. A silent no-op
// would leave a "defective" variant identical to the netlist it was derived
// from, which is a fixture that proves nothing while still running.
inline std::string replaceOnce(std::string text,
                               const std::string& from,
                               const std::string& to)
{
  const std::string::size_type at = text.find(from);
  if (at == std::string::npos) {
    throw std::logic_error("TestNetlists: '" + from
                           + "' is not in the netlist any more; the derived "
                             "variant would be identical to it");
  }
  return text.replace(at, from.size(), to);
}

// kHierNetlist with block1's u2 changed from a buffer to an inverter, so it is
// genuinely not equivalent. block2 holds an identical u2 line, hence the
// first-occurrence replacement: block1 is the one declared first.
inline std::string counterexampleNetlist()
{
  return replaceOnce(kHierNetlist,
                     "BUF_X1 u2 (.A(r1q), .Z(out));",
                     "INV_X1 u2 (.A(r1q), .ZN(out));");
}

// kHierNetlist with top's `out2` output port removed and its driver left
// unconnected. The logic on the remaining outputs is unchanged.
//
// This is the shape a hierarchy-writer bug takes when it drops a port or a
// connection, and it is the case an equivalence check most easily gets wrong:
// comparing only the intersection of the two designs' boundary points reports
// "no difference" here.
inline std::string droppedPortNetlist()
{
  std::string v = kHierNetlist;
  v = replaceOnce(v,
                  "module top (in, clk1, clk2, out, out2);",
                  "module top (in, clk1, clk2, out);");
  v = replaceOnce(
      v, "   output out, out2;\n\n   block1", "   output out;\n\n   block1");
  return replaceOnce(v, ".out(b1out), .out2(out2));", ".out(b1out), .out2());");
}

// Writes `text` into `work_dir` and returns the path.
inline std::filesystem::path writeNetlist(const std::filesystem::path& work_dir,
                                          const std::string& name,
                                          const std::string& text)
{
  const std::filesystem::path path = work_dir / name;
  std::ofstream out(path);
  out << text;
  return path;
}

}  // namespace tst
