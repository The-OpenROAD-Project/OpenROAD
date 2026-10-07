// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// generate_regfile: a multi-port register file as a placed standard-cell
// macro, built directly in odb.
//
// A register file synthesised to flops is a flop per bit, a W-way write
// mux on each flop's D and an R-way read mux tree per bit: dense, and the
// legaliser cannot spread it. Here it is laid out the way a bitcell array
// is: word rows by bit columns, one tile per (word, bit) holding the flop,
// its write select and R read AND gates; the read wordlines run along the
// rows and are decoded in a header column, the read bitlines are OR trees
// in a footer row. Every cell is placed by construction; nothing is left
// to a placer.
#pragma once

#include <string>
#include <vector>

namespace odb {
class dbDatabase;
class dbBlock;
class dbMaster;
class dbNet;
class dbInst;
}  // namespace odb

namespace utl {
class Logger;
}

namespace ram::regfile {

// One read or write port, with the RTL's own pin names so the macro drops
// in for the module it replaces.
struct Port
{
  std::string addr;  // bus name; bits are <addr>[i]
  std::string data;  // bus name; bits are <data>[i]
  std::string en;    // write ports only; empty means always enabled
  // A banked read port (Chisel RegfileBank): one address and one data bus
  // per bank, the bank mux outside. addr/data above are unused then.
  std::vector<std::string> bank_addr;
  std::vector<std::string> bank_data;
  bool banked() const { return !bank_addr.empty(); }
};

// The cells the array is built from, by name in the loaded LEF.
struct Cells
{
  std::string flop;  // D flip-flop, Q output (QN accepted, see below)
  // the storage flop with `async_reset`: an asynchronous reset (RESETN)
  // and set (SETN, tied off) flop
  std::string flop_r;
  std::string tie_hi;  // ties flop_r's set off, one per word
  std::string and2;    // read select AND
  std::string or2;     // bitline OR tree node
  std::string ao22;    // write mux node: (sel & new) | (hold & old)
  // read bitline: AOI22 leaf per word pair, NAND2/NOR2 tree over them
  std::string aoi22 = "AOI22xp5_ASAP7_75t_R";
  std::string nand2 = "NAND2xp5_ASAP7_75t_R";
  std::string nor2 = "NOR2xp33_ASAP7_75t_R";
  std::string inv;  // decoder inverters
  std::string tap;  // well tap, one column per tap_columns
  // integrated clock gate, one per word with `write_style clock_gate`
  std::string icg = "ICGx1_ASAP7_75t_R";
  std::string tie_lo;  // for unused inputs
};

// Knobs of the model liberty (views.h), asap7-shaped defaults, set in
// the spec with `lib <knob> <value>`. Picoseconds, femtofarads,
// nanowatts, so a spec reads in plain units.
struct LibModel
{
  double gate_delay_ps = 25.0;  // one loaded gate level
  double wire_factor = 1.3;     // on top of the gate levels
  double input_load_ff = 0.6;   // per driven cell input
  double clock_load_ff = 10.0;  // the block's clock pin as the parent sees it
  double leakage_nw_per_cell = 1.0;
  double output_max_cap_ff = 50.0;
  double hold_ps = 0.0;
};

struct Spec
{
  // "macro" (an abstract and a model liberty for a parent to place) or
  // "netlist" (the placed cells dropped into the parent's rows by the
  // flow); the generator builds the same block either way. The flow's
  // memories step reads this key too, and decides which views to ask for.
  std::string mode = "macro";
  std::string module;  // generated module and block name
  int words = 0;
  int bits = 0;
  std::string clock = "clock";
  std::string reset;  // optional: a port the RTL has and the array ignores
  // `async_reset <port> low|high`: the reset clears every stored word to
  // zero asynchronously, as an RTL `always @(posedge clk or negedge
  // rst_n)` register file does; the storage flops are `cell flop_r`. The
  // port is `reset` above. Read-address registers (read_latency) are not
  // reset.
  bool async_reset = false;
  bool reset_active_low = true;
  // `unused <port>`: one-bit inputs the RTL module has and the array
  // ignores (ibex's test_en_i), so the block has the module's ports.
  std::vector<std::string> unused;
  std::vector<Port> read;
  std::vector<Port> write;
  Cells cells;
  int tap_columns = 8;     // a tap column every N bit columns
  int service_sites = 40;  // free sites beside each tap, for the clock tree
  int banks = 1;           // word columns side by side; words % banks == 0
  // How many of the banks stand side by side; the rest stack below them.
  // 0 means all of them, one row of banks. The outline is bank_columns
  // bank widths wide and banks/bank_columns bank heights tall: a wide
  // word (128 bits) wants its banks stacked, a narrow one wants them
  // side by side. banks % bank_columns == 0.
  int bank_columns = 0;
  // The word split into this many bit bands, stacked: a 564-bit word
  // is one band 564 tiles wide, or four bands of 141 one above the
  // other, each with its own copy of the word decode; the last band is
  // short when the folds do not divide. Banks fold words; this folds bits.
  int bit_folds = 1;
  // Read latency in cycles: 0 reads combinationally from the address
  // ports; 1 registers each read address on the clock first, the way
  // firtool writes a Chisel register file read through RegNext(addr)
  // (XiangShan's IntRegFile, FpRegFile and VfRegFile). The macro
  // replaces the whole module, so it has to carry that register.
  int read_latency = 0;
  // How words fall into banks: "contiguous" (bank n / words_per_bank,
  // the default) or "interleaved" (bank n % banks, entry n / banks, as
  // XiangShan's IntRegFile banks its read: bank k's entry l is word
  // l * banks + k).
  std::string bank_order = "contiguous";
  // A word that is the constant zero and has no storage (RISC-V x0): it
  // reads 0 and a write to it is dropped. -1 for none.
  int zero_word = -1;
  // How a word keeps its value between writes: "mux" (the default), a
  // free-running clock and a hold term in every bit's write mux; or
  // "clock_gate", one integrated clock gate per word enabled by any of
  // its write selects, as generate_ram writes, so a bit's D is the write
  // data itself with one write port and a select mux of the writes with
  // more.
  std::string write_style = "mux";
  // Instance names of the flops, as patterns, so a flop carries the name
  // of the RTL register bit it implements (an equivalence check matches
  // sequential instances by name). {word} {bit} for the storage,
  // {port} {bank} {bit} for a read address register. Empty keeps the
  // generator's own names.
  std::string store_name;
  std::string read_reg_name;
  LibModel lib;
  // Pins: left and right edges on the horizontal layer, top and bottom on
  // the vertical one, centred on that layer's track grid as the platform
  // makes it, so a parent's macro placer can align them.
  std::string pin_layer_h = "M4";
  std::string pin_layer_v = "M5";
  double pin_track_offset_um = 0.012;
  double pin_track_pitch_um = 0.048;
};

// Reads a spec from a small `key value` text file (see README.md).
// Throws std::runtime_error with the offending line on anything it does
// not understand: a spec is either complete or refused.
Spec ReadSpec(const std::string& path);

// Builds the netlist and placement of `spec` into a new block of `db`,
// whose tech and libs must already hold the cells named in the spec.
// Returns the block. Throws std::runtime_error naming the cell or pin
// when the library does not fit the spec.
odb::dbBlock* Generate(odb::dbDatabase* db,
                       utl::Logger* logger,
                       const Spec& spec);

// Writes the block as a structural Verilog module, cells as instances,
// for simulation and for the flow's netlist view.
void WriteVerilog(odb::dbBlock* block, const std::string& path);

}  // namespace ram::regfile
