// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The performance baseline: times the solver at its default tolerances (1% on
// the total, 5% on couplings of at least 1% of it) on three groups of cases,
// solving every net of each case in turn:
// - 3d: the golden 3D structures, and two golden cross sections extruded to
//   10 um lines (planar and conformal);
// - 2d: the golden cross sections, per unit length;
// - nets: nets of a routed design within a halo, by default the trimmed
//   sky130hd gcd of field_solver_gcd.
// Each solve reports its time, walks, hops per walk, the relative standard
// error of one walk (sigma / total * sqrt(walks), which the walk count scales
// with), the start points rejected inside the Gaussian surface, the window's
// leakage, the scene's box count and its smallest box dimension (which sets
// the capture distance). A short warm-up solve of every net first builds
// the tables the case needs; its time is reported apart, and case times are
// warm.
//
// bazelisk run starts it in its runfiles, where the default files are; give
// other files as absolute paths:
//   bazelisk run //src/fsv/test:BenchSuite -- [--threads N]
//       [--groups 3d,2d,nets] [--csv FILE]
//       [--lef FILE]... [--def FILE] [--itf FILE] [--nets a,b,...]
//       [--halo UM] [--margin UM]

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Structures.h"
#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/NetWindow.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/lefin.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  int threads = 1;
  std::vector<std::string> groups{"3d", "2d", "nets"};
  std::string csv;
  std::vector<std::string> lefs{
      "src/fsv/test/sky130hd/sky130hd.tlef",
      "src/fsv/test/sky130hd/sky130_fd_sc_hd_merged.lef"};
  std::string def = "src/fsv/test/field_solver_gcd.def";
  std::string itf = "src/fsv/test/sky130.itf";
  std::vector<std::string> nets{"_042_", "_119_/_067_", "req_msg[10]"};
  double halo = 4;
  double margin = 1;
};

std::vector<std::string> split(const std::string& list)
{
  std::vector<std::string> out;
  std::stringstream in(list);
  std::string item;
  while (std::getline(in, item, ',')) {
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

Options parse(const int argc, char** argv)
{
  Options o;
  bool lefs_given = false;
  for (int i = 1; i < argc; i++) {
    const std::string_view arg = argv[i];
    auto value = [&]() -> std::string {
      if (i + 1 >= argc) {
        throw std::invalid_argument(std::string(arg) + " needs a value");
      }
      return argv[++i];
    };
    if (arg == "--threads") {
      o.threads = std::stoi(value());
    } else if (arg == "--groups") {
      o.groups = split(value());
    } else if (arg == "--csv") {
      o.csv = value();
    } else if (arg == "--lef") {
      if (!lefs_given) {
        o.lefs.clear();
        lefs_given = true;
      }
      o.lefs.push_back(value());
    } else if (arg == "--def") {
      o.def = value();
    } else if (arg == "--itf") {
      o.itf = value();
    } else if (arg == "--nets") {
      o.nets = split(value());
    } else if (arg == "--halo") {
      o.halo = std::stod(value());
    } else if (arg == "--margin") {
      o.margin = std::stod(value());
    } else {
      throw std::invalid_argument("unknown argument " + std::string(arg));
    }
  }
  if (o.groups.empty()) {
    throw std::invalid_argument("--groups needs at least one group");
  }
  for (const std::string& g : o.groups) {
    if (g != "3d" && g != "2d" && g != "nets") {
      throw std::invalid_argument("unknown group " + g
                                  + "; the groups are 3d, 2d and nets");
    }
  }
  return o;
}

bool wanted(const Options& o, const std::string& group)
{
  for (const std::string& g : o.groups) {
    if (g == group) {
      return true;
    }
  }
  return false;
}

struct Row
{
  std::string group;
  std::string name;
  std::string net;
  double seconds;
  fsv::CapRow row;
  int boxes;
  double min_dim;
};

double seconds(const std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
      .count();
}

template <class SceneType, class GeometryType>
void solveAll(fsv::Solver& solver,
              const std::string& group,
              const std::string& name,
              const SceneType& scene,
              const GeometryType& geometry,
              const std::vector<int>& targets,
              const int boxes,
              std::vector<Row>& rows)
{
  for (const int net : targets) {
    const auto start = std::chrono::steady_clock::now();
    const fsv::CapRow row = solver.solve(scene, net, {});
    rows.push_back({group,
                    name,
                    geometry.netName(net),
                    seconds(start),
                    row,
                    boxes,
                    scene.minDimension()});
  }
}

std::vector<int> allNets(const int count)
{
  std::vector<int> nets(count);
  for (int i = 0; i < count; i++) {
    nets[i] = i;
  }
  return nets;
}

// Tables are built when a walk first meets a permittivity ratio, so a short
// solve of every net builds those the timed solves will need.
template <class SceneType>
double warmUp(fsv::Solver& solver,
              const SceneType& scene,
              const std::vector<int>& targets)
{
  fsv::SolveOptions options;
  options.min_walks = options.max_walks = 4096;
  const auto start = std::chrono::steady_clock::now();
  for (const int net : targets) {
    solver.solve(scene, net, options);
  }
  return seconds(start);
}

void runStructures(fsv::Solver& solver, std::vector<Row>& rows, double& tables)
{
  struct Case
  {
    std::string structure;
    double length;  // > 0: a cross section extruded to a line
  };
  const std::vector<Case> cases{{"crossing", 0},
                                {"conformal_line_ends", 0},
                                {"conformal_crossing", 0},
                                {"etch_stop_pair", 10},
                                {"conformal_pair", 10}};
  for (const Case& c : cases) {
    const fsv::Structure s = fsv::readStructure(c.structure, c.length);
    const fsv::Scene scene(s.geometry, s.stack);
    const std::vector<int> nets = allNets(s.geometry.netCount());
    tables += warmUp(solver, scene, nets);
    const std::string name
        = c.length > 0 ? c.structure + " (10 um line)" : c.structure;
    solveAll(solver,
             "3d",
             name,
             scene,
             s.geometry,
             nets,
             static_cast<int>(s.geometry.boxes().size()),
             rows);
  }
}

void runCrossSections(fsv::Solver& solver,
                      std::vector<Row>& rows,
                      double& tables)
{
  for (const char* name : {"etch_stop_pair",
                           "three_wires",
                           "conformal_pair",
                           "nested_conformal",
                           "merged_conformal",
                           "sky130_met1_pair"}) {
    const fsv::Structure s = fsv::readStructure(name);
    const fsv::Geometry2D section = fsv::crossSection(s.geometry);
    const fsv::Scene2D scene(section, s.stack);
    const std::vector<int> nets = allNets(section.netCount());
    tables += warmUp(solver, scene, nets);
    solveAll(solver,
             "2d",
             name,
             scene,
             section,
             nets,
             static_cast<int>(section.rects().size()),
             rows);
  }
}

void runNets(const Options& o,
             fsv::Solver& solver,
             std::vector<Row>& rows,
             double& tables)
{
  utl::Logger logger;
  const std::unique_ptr<odb::dbDatabase, void (*)(odb::dbDatabase*)> owner(
      odb::dbDatabase::create(), &odb::dbDatabase::destroy);
  odb::dbDatabase* db = owner.get();
  odb::lefin lef_reader(db, &logger, false);
  std::vector<odb::dbLib*> libs;
  odb::dbTech* tech = nullptr;
  for (size_t i = 0; i < o.lefs.size(); i++) {
    const std::string lib_name = "lib" + std::to_string(i);
    odb::dbLib* lib = i == 0 ? lef_reader.createTechAndLib(
                                   "tech", lib_name.c_str(), o.lefs[i].c_str())
                             : lef_reader.createLib(
                                   tech, lib_name.c_str(), o.lefs[i].c_str());
    if (lib == nullptr) {
      throw std::runtime_error("cannot read " + o.lefs[i]);
    }
    tech = lib->getTech();
    libs.push_back(lib);
  }
  odb::dbChip* chip = odb::dbChip::create(db, tech);
  odb::defin def_reader(db, &logger);
  if (!def_reader.readChip(libs, o.def.c_str(), chip)) {
    throw std::runtime_error("cannot read " + o.def);
  }
  odb::dbBlock* block = chip->getBlock();
  const fsv::Stack stack = fsv::readItfFile(o.itf);
  const fsv::LayerExtent extent = [&](const std::string_view layer) {
    const fsv::Stack::ZExtent z = stack.zExtent(layer);
    if (z.status != fsv::Stack::ZExtent::kFound) {
      throw std::invalid_argument("layer " + std::string(layer)
                                  + " has no z extent in the stack");
    }
    return std::make_pair(z.bottom, z.top);
  };
  for (const std::string& name : o.nets) {
    odb::dbNet* net = block->findNet(name.c_str());
    if (net == nullptr) {
      throw std::invalid_argument("net " + name + " is not in the design");
    }
    const fsv::NetWindow window
        = fsv::readNetWindow(block, {net}, extent, {o.halo, o.margin});
    const fsv::Scene scene(window.geometry, stack);
    tables += warmUp(solver, scene, window.targets);
    solveAll(solver,
             "nets",
             name,
             scene,
             window.geometry,
             window.targets,
             static_cast<int>(window.geometry.boxes().size()),
             rows);
  }
}

void report(const std::vector<Row>& rows,
            const double tables,
            const int threads,
            FILE* out)
{
  std::printf("%-5s %-28s %-14s %9s %10s %9s %8s %8s %7s %6s %8s\n",
              "group",
              "case",
              "net",
              "seconds",
              "walks",
              "hops/walk",
              "sigma1",
              "reject%",
              "leak%",
              "boxes",
              "min_dim");
  if (out != nullptr) {
    std::fprintf(out,
                 "group,case,net,threads,seconds,walks,hops,total,sigma,"
                 "rejected,window_leakage,boxes,min_dim\n");
  }
  std::string group;
  double log_sum = 0;
  int count = 0;
  auto summary = [&] {
    if (count > 0) {
      std::printf("%-5s geometric mean %.3f s over %d solves\n",
                  group.c_str(),
                  std::exp(log_sum / count),
                  count);
    }
  };
  for (const Row& r : rows) {
    if (r.group != group) {
      summary();
      group = r.group;
      log_sum = 0;
      count = 0;
    }
    const fsv::CapRow& row = r.row;
    const double total = row.total();
    const double sigma1 = row.sigma[row.master] / total
                          * std::sqrt(static_cast<double>(row.walks));
    std::printf(
        "%-5s %-28s %-14s %9.3f %10lld %9.1f %8.2f %8.1f %7.2f %6d %8.3g\n",
        r.group.c_str(),
        r.name.c_str(),
        r.net.c_str(),
        r.seconds,
        static_cast<long long>(row.walks),
        static_cast<double>(row.hops) / row.walks,
        sigma1,
        100.0 * row.rejected / row.walks,
        100.0 * row.window_leakage / total,
        r.boxes,
        r.min_dim);
    if (out != nullptr) {
      std::fprintf(out,
                   "%s,\"%s\",\"%s\",%d,%.6f,%lld,%lld,%.8g,%.8g,%lld,%.8g,%d,"
                   "%.8g\n",
                   r.group.c_str(),
                   r.name.c_str(),
                   r.net.c_str(),
                   threads,
                   r.seconds,
                   static_cast<long long>(row.walks),
                   static_cast<long long>(row.hops),
                   total,
                   row.sigma[row.master],
                   static_cast<long long>(row.rejected),
                   row.window_leakage,
                   r.boxes,
                   r.min_dim);
    }
    // A solve too fast for the clock would make the log -inf.
    log_sum += std::log(std::max(r.seconds, 1e-9));
    count++;
  }
  summary();
  std::printf(
      "warm-up (tables and a short solve of each net) %.3f s; %d "
      "threads\n",
      tables,
      threads);
}

}  // namespace

int main(const int argc, char** argv)
{
  try {
    const Options o = parse(argc, argv);
    // Opened first, so that a bad path fails before the solves.
    FILE* csv = nullptr;
    if (!o.csv.empty()) {
      csv = std::fopen(o.csv.c_str(), "w");
      if (csv == nullptr) {
        throw std::runtime_error("cannot write " + o.csv + ": "
                                 + std::strerror(errno));
      }
    }
    std::unique_ptr<FILE, int (*)(FILE*)> csv_owner(
        csv, [](FILE* f) { return f == nullptr ? 0 : std::fclose(f); });
    fsv::Solver solver({.threads = o.threads});
    std::vector<Row> rows;
    double tables = 0;
    if (wanted(o, "3d")) {
      runStructures(solver, rows, tables);
    }
    if (wanted(o, "2d")) {
      runCrossSections(solver, rows, tables);
    }
    if (wanted(o, "nets")) {
      runNets(o, solver, rows, tables);
    }
    report(rows, tables, o.threads, csv);
    if (csv != nullptr
        && (std::ferror(csv) != 0 || std::fclose(csv_owner.release()) != 0)) {
      throw std::runtime_error("cannot write " + o.csv);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "BenchSuite: %s\n", e.what());
    return 1;
  }
  return 0;
}
