// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// generate_regfile: abstract LEF, model liberty, and the spec-against-RTL
// port check. See views.h.
#include "views.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/lefout.h"
#include "regfile.h"
#include "utl/Logger.h"

namespace ram::regfile {

namespace {

int Log2Ceil(int n)
{
  int k = 0;
  while ((1 << k) < n) {
    ++k;
  }
  return k;
}

int AddrWidth(int words)
{
  return std::max(Log2Ceil(words), 1);
}

std::string F(double v)
{
  std::ostringstream s;
  s.precision(6);
  s << std::fixed << v;
  return s.str();
}

// The two-point tables FakeRAM's libs use: constant across slew and load,
// which is what a model is.
void Table2(std::ostream& o,
            const char* kind,
            const std::string& tmpl,
            double value_ns,
            int indent)
{
  std::string pad(indent, ' ');
  o << pad << kind << "(" << tmpl << ") {\n"
    << pad << "    index_1 (\"0.009, 0.227\");\n"
    << pad << "    index_2 (\"0.005, 0.500\");\n"
    << pad << "    values ( \\\n"
    << pad << "      \"" << F(value_ns) << ", " << F(value_ns) << "\", \\\n"
    << pad << "      \"" << F(value_ns) << ", " << F(value_ns) << "\" \\\n"
    << pad << "    )\n"
    << pad << "}\n";
}

void Constraint(std::ostream& o,
                const std::string& cell,
                const std::string& clk,
                double setup_ns,
                double hold_ns)
{
  o << "        timing() {\n"
    << "            related_pin : " << clk << ";\n"
    << "            timing_type : setup_rising ;\n";
  Table2(o, "rise_constraint", cell + "_constraint_template", setup_ns, 12);
  Table2(o, "fall_constraint", cell + "_constraint_template", setup_ns, 12);
  o << "        }\n"
    << "        timing() {\n"
    << "            related_pin : " << clk << ";\n"
    << "            timing_type : hold_rising ;\n";
  Table2(o, "rise_constraint", cell + "_constraint_template", hold_ns, 12);
  Table2(o, "fall_constraint", cell + "_constraint_template", hold_ns, 12);
  o << "        }\n";
}

}  // namespace

void WriteLef(odb::dbBlock* block, utl::Logger* logger, const std::string& path)
{
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("cannot write " + path);
  }
  odb::lefout writer(logger, out);
  writer.setBloatOccupiedLayers(true);
  writer.writeAbstractLef(block);
}

void WriteLiberty(odb::dbBlock* block,
                  const Spec& spec,
                  const LibModel& m,
                  const std::string& path,
                  bool pre_layout)
{
  std::ofstream o(path);
  if (!o) {
    throw std::runtime_error("cannot write " + path);
  }
  const std::string cell = spec.module;
  const int A = AddrWidth(spec.words);
  const int R = static_cast<int>(spec.read.size());
  const int W = static_cast<int>(spec.write.size());
  const int banks = std::max(spec.banks, 1);
  const int words_per_bank = spec.words / banks;

  // Gate levels, from the structure the generator built (regfile.cc).
  const int read_levels
      = 1 + Log2Ceil(A) + 1 + Log2Ceil(words_per_bank) + Log2Ceil(banks);
  const int write_levels = 1 + Log2Ceil(A + 1) + 1 + Log2Ceil((W + 2) / 2);
  const double read_ns = read_levels * m.gate_delay_ps * m.wire_factor / 1000.0;
  const double setup_ns
      = write_levels * m.gate_delay_ps * m.wire_factor / 1000.0;
  const double hold_ns = m.hold_ps / 1000.0;
  // Loads, from fanout: an address literal reaches one inverter and half
  // the words' decodes; a data bit reaches every word's write mux; an
  // enable reaches every word's decode.
  const double addr_pf = (spec.words / 2 + 1) * m.input_load_ff / 1000.0;
  const double data_pf = spec.words * m.input_load_ff / 1000.0;
  const double en_pf = spec.words * m.input_load_ff / 1000.0;
  const double clk_pf = m.clock_load_ff / 1000.0;
  const odb::Rect die = block->getDieArea();
  const double dbu = block->getDbUnitsPerMicron();
  const double area_um2 = die.dx() / dbu * (die.dy() / dbu);
  const double leakage_uw
      = block->getInsts().size() * m.leakage_nw_per_cell / 1000.0;

  o << "library(" << cell << ") {\n"
    << "    technology (cmos);\n"
    << "    delay_model : table_lookup;\n"
    << "    revision : 1.0;\n"
    << "    comment : \"generate_regfile model"
    << (pre_layout ? ", pre-layout" : "")
    << ": loads from fanout, arcs from gate depth; replaced by the routed "
       "block's abstract for a measured run\";\n"
    << "    time_unit : \"1ns\";\n"
    << "    voltage_unit : \"1V\";\n"
    << "    current_unit : \"1uA\";\n"
    << "    leakage_power_unit : \"1uW\";\n"
    << "    nom_process : 1;\n"
    << "    nom_temperature : 25.000;\n"
    << "    nom_voltage : 0.700;\n"
    << "    capacitive_load_unit (1,pf);\n"
    << "    pulling_resistance_unit : \"1kohm\";\n"
    << "    operating_conditions(tt_1.0_25.0) {\n"
    << "        process : 1;\n"
    << "        temperature : 25.000;\n"
    << "        voltage : 0.700;\n"
    << "        tree_type : balanced_tree;\n"
    << "    }\n"
    << "    default_cell_leakage_power : 0;\n"
    << "    default_fanout_load : 1;\n"
    << "    default_inout_pin_cap : 0.0;\n"
    << "    default_input_pin_cap : 0.0;\n"
    << "    default_output_pin_cap : 0.0;\n"
    << "    default_max_transition : 0.227;\n"
    << "    default_operating_conditions : tt_1.0_25.0;\n"
    << "    default_leakage_power_density : 0.0;\n"
    << "    slew_derate_from_library : 1.000;\n"
    << "    slew_lower_threshold_pct_fall : 20.000;\n"
    << "    slew_upper_threshold_pct_fall : 80.000;\n"
    << "    slew_lower_threshold_pct_rise : 20.000;\n"
    << "    slew_upper_threshold_pct_rise : 80.000;\n"
    << "    input_threshold_pct_fall : 50.000;\n"
    << "    input_threshold_pct_rise : 50.000;\n"
    << "    output_threshold_pct_fall : 50.000;\n"
    << "    output_threshold_pct_rise : 50.000;\n"
    << "    lu_table_template(" << cell << "_delay_template) {\n"
    << "        variable_1 : input_net_transition;\n"
    << "        variable_2 : total_output_net_capacitance;\n"
    << "        index_1 (\"1000, 1001\");\n"
    << "        index_2 (\"1000, 1001\");\n"
    << "    }\n"
    << "    lu_table_template(" << cell << "_slew_template) {\n"
    << "        variable_1 : total_output_net_capacitance;\n"
    << "        index_1 (\"1000, 1001\");\n"
    << "    }\n"
    << "    lu_table_template(" << cell << "_constraint_template) {\n"
    << "        variable_1 : related_pin_transition;\n"
    << "        variable_2 : constrained_pin_transition;\n"
    << "        index_1 (\"1000, 1001\");\n"
    << "        index_2 (\"1000, 1001\");\n"
    << "    }\n"
    << "    library_features(report_delay_calculation);\n";
  const int A_bank = AddrWidth(words_per_bank);
  // One bus type per width in use.
  std::set<int> widths = {A, spec.bits, A_bank};
  for (int w : widths) {
    o << "    type (" << cell << "_bus_" << w << ") {\n"
      << "        base_type : array ;\n"
      << "        data_type : bit ;\n"
      << "        bit_width : " << w << ";\n"
      << "        bit_from : " << w - 1 << ";\n"
      << "        bit_to : 0 ;\n"
      << "        downto : true ;\n"
      << "    }\n";
  }
  o << "cell(" << cell << ") {\n"
    << "    area : " << F(area_um2) << ";\n"
    << "    is_macro_cell : true;\n"
    << "    interface_timing : true;\n";
  // Clock.
  o << "    pin(" << spec.clock << ")   {\n"
    << "        direction : input;\n"
    << "        capacitance : " << F(clk_pf) << ";\n"
    << "        clock : true;\n"
    << "    }\n";
  if (!spec.reset.empty()) {
    o << "    pin(" << spec.reset << ")   {\n"
      << "        direction : input;\n"
      << "        capacitance : 0.001000;\n"
      << "    }\n";
  }
  // Read ports: address in, data out combinational from that address; with
  // a read latency the address is a flop's D (setup and hold to the clock)
  // and the data comes from the clock, one flop level more. A banked port
  // is one such pair per bank, with the bank-local width.
  const bool registered = spec.read_latency > 0;
  const double flop_ns = m.gate_delay_ps * m.wire_factor / 1000.0;
  auto read_pair = [&](const std::string& addr,
                       const std::string& data,
                       int a_width,
                       double read_ns_here) {
    o << "    bus(" << addr << ")   {\n"
      << "        bus_type : " << cell << "_bus_" << a_width << ";\n"
      << "        direction : input;\n"
      << "        capacitance : "
      << F(registered ? m.input_load_ff / 1000.0 : addr_pf) << ";\n";
    if (registered) {
      Constraint(o, cell, spec.clock, flop_ns, hold_ns);
    }
    o << "    }\n"
      << "    bus(" << data << ")   {\n"
      << "        bus_type : " << cell << "_bus_" << spec.bits << ";\n"
      << "        direction : output;\n"
      << "        max_capacitance : " << F(m.output_max_cap_ff / 1000.0)
      << ";\n";
    // The arc per data bit, related to every address bit: a bus-to-bus
    // arc between an A-bit address and a B-bit data would be read as
    // bitwise pairs of different sizes (STA-1216). Every data bit
    // depends on every address bit here.
    std::string addr_bits;
    for (int i = 0; i < a_width; ++i) {
      addr_bits += (i ? " " : "") + addr + "[" + std::to_string(i) + "]";
    }
    for (int b = 0; b < spec.bits; ++b) {
      o << "        pin(" << data << "[" << b << "]) {\n"
        << "            direction : output;\n"
        << "            timing() {\n";
      if (registered) {
        o << "                related_pin : " << spec.clock << " ;\n"
          << "                timing_type : rising_edge;\n";
      } else {
        o << "                related_pin : \"" << addr_bits << "\" ;\n"
          << "                timing_type : combinational;\n"
          << "                timing_sense : non_unate;\n";
      }
      const double arc_ns = read_ns_here + (registered ? flop_ns : 0.0);
      Table2(o, "cell_rise", cell + "_delay_template", arc_ns, 16);
      Table2(o, "cell_fall", cell + "_delay_template", arc_ns, 16);
      o << "                rise_transition(" << cell << "_slew_template) {\n"
        << "                    index_1 (\"0.005, 0.500\");\n"
        << "                    values (\"0.009, 0.227\")\n"
        << "                }\n"
        << "                fall_transition(" << cell << "_slew_template) {\n"
        << "                    index_1 (\"0.005, 0.500\");\n"
        << "                    values (\"0.009, 0.227\")\n"
        << "                }\n"
        << "            }\n"
        << "        }\n";
    }
    o << "    }\n";
  };
  // A banked port's read has no footer level.
  const int bank_read_levels
      = 1 + Log2Ceil(A_bank) + 1 + Log2Ceil(words_per_bank);
  const double bank_read_ns
      = bank_read_levels * m.gate_delay_ps * m.wire_factor / 1000.0;
  for (int r = 0; r < R; ++r) {
    if (spec.read[r].banked()) {
      for (size_t k = 0; k < spec.read[r].bank_addr.size(); ++k) {
        read_pair(spec.read[r].bank_addr[k],
                  spec.read[r].bank_data[k],
                  A_bank,
                  bank_read_ns);
      }
    } else {
      read_pair(spec.read[r].addr, spec.read[r].data, A, read_ns);
    }
  }
  // Write ports: everything constrained to the clock.
  for (int w = 0; w < W; ++w) {
    o << "    bus(" << spec.write[w].addr << ")   {\n"
      << "        bus_type : " << cell << "_bus_" << A << ";\n"
      << "        direction : input;\n"
      << "        capacitance : " << F(addr_pf) << ";\n";
    Constraint(o, cell, spec.clock, setup_ns, hold_ns);
    o << "    }\n"
      << "    bus(" << spec.write[w].data << ")   {\n"
      << "        bus_type : " << cell << "_bus_" << spec.bits << ";\n"
      << "        direction : input;\n"
      << "        capacitance : " << F(data_pf) << ";\n";
    Constraint(o, cell, spec.clock, setup_ns, hold_ns);
    o << "    }\n";
    if (!spec.write[w].en.empty()) {
      o << "    pin(" << spec.write[w].en << ")   {\n"
        << "        direction : input;\n"
        << "        capacitance : " << F(en_pf) << ";\n";
      Constraint(o, cell, spec.clock, setup_ns, hold_ns);
      o << "    }\n";
    }
  }
  o << "    cell_leakage_power : " << F(leakage_uw) << ";\n"
    << "}\n"
    << "}\n";
}

std::vector<RtlPort> ReadModulePorts(const std::string& verilog,
                                     const std::string& module)
{
  std::ifstream in(verilog);
  if (!in) {
    throw std::runtime_error("cannot read " + verilog);
  }
  std::string text((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  std::regex head("\\bmodule\\s+" + module + "\\s*\\(");
  std::smatch mh;
  if (!std::regex_search(text, mh, head)) {
    throw std::runtime_error("module " + module + " is not in " + verilog);
  }
  size_t start = mh.position(0) + mh.length(0);
  size_t end = text.find(");", start);
  if (end == std::string::npos) {
    throw std::runtime_error("module " + module + ": header never closes");
  }
  std::string header = text.substr(start, end - start);
  // Strip comments.
  header = std::regex_replace(header, std::regex("//[^\n]*"), "");
  header = std::regex_replace(header, std::regex("/\\*[\\s\\S]*?\\*/"), "");
  // ANSI ports, one entry per comma: `input [4:0] a, b` declares two, the
  // second inheriting the first's direction and width, whether it is on
  // the same line or the next.
  std::vector<RtlPort> ports;
  std::regex decl(
      "^\\s*(input|output|inout)?\\s*(?:wire|reg|logic)?\\s*"
      "(\\[\\s*(\\d+)\\s*:\\s*(\\d+)\\s*\\])?\\s*"
      "([A-Za-z_][A-Za-z_0-9$]*)\\s*$");
  bool input = true;
  int width = 1;
  std::istringstream entries(header);
  std::string entry;
  while (std::getline(entries, entry, ',')) {
    std::smatch m;
    if (!std::regex_match(entry, m, decl)) {
      continue;
    }
    if (m[1].matched) {
      input = m[1].str() != "output";
      width = 1;
    }
    if (m[2].matched) {
      width = std::abs(std::stoi(m[3].str()) - std::stoi(m[4].str())) + 1;
    }
    ports.push_back(RtlPort{m[5].str(), width, input});
  }
  return ports;
}

std::vector<std::string> CheckPorts(const Spec& spec,
                                    const std::vector<RtlPort>& ports)
{
  const int A = AddrWidth(spec.words);
  std::map<std::string, RtlPort> want;
  want[spec.clock] = RtlPort{spec.clock, 1, true};
  if (!spec.reset.empty()) {
    want[spec.reset] = RtlPort{spec.reset, 1, true};
  }
  const int banks = std::max(spec.banks, 1);
  const int A_bank = AddrWidth(spec.words / banks);
  for (const auto& r : spec.read) {
    if (r.banked()) {
      for (size_t k = 0; k < r.bank_addr.size(); ++k) {
        want[r.bank_addr[k]] = RtlPort{r.bank_addr[k], A_bank, true};
        want[r.bank_data[k]] = RtlPort{r.bank_data[k], spec.bits, false};
      }
    } else {
      want[r.addr] = RtlPort{r.addr, A, true};
      want[r.data] = RtlPort{r.data, spec.bits, false};
    }
  }
  for (const auto& w : spec.write) {
    want[w.addr] = RtlPort{w.addr, A, true};
    want[w.data] = RtlPort{w.data, spec.bits, true};
    if (!w.en.empty()) {
      want[w.en] = RtlPort{w.en, 1, true};
    }
  }
  std::map<std::string, RtlPort> have;
  for (const auto& p : ports) {
    have[p.name] = p;
  }
  std::vector<std::string> problems;
  for (const auto& [name, w] : want) {
    auto it = have.find(name);
    if (it == have.end()) {
      problems.push_back("spec names port " + name + ", the module has none");
      continue;
    }
    if (it->second.width != w.width) {
      problems.push_back(
          "port " + name + " is " + std::to_string(it->second.width)
          + " bits in the module, " + std::to_string(w.width) + " in the spec");
    }
    if (it->second.input != w.input) {
      problems.push_back("port " + name + " is "
                         + (it->second.input ? "an input" : "an output")
                         + " in the module and the other way in the spec");
    }
  }
  for (const auto& [name, p] : have) {
    if (want.find(name) == want.end()) {
      problems.push_back("module port " + name + " is not in the spec");
    }
  }
  return problems;
}

}  // namespace ram::regfile
