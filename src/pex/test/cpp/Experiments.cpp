// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The modeling experiments: each measures, against fsv, the error of one
// simplification the extractor's model makes, on one conductor layer of a
// process stack. See README.md for what each decides.
// - e1 additivity: a wire whose neighbor and cover change along it,
//   against the sum of its pieces plus the end terms.
// - e2 crossing: a wire under or over an array of crossing wires, against
//   the plane × coverage rule.
// - e3 ends: the capacitance of short wires against per-unit-length ×
//   length, and two line ends facing each other.
// - e4 lateral: neighbors beyond the nearest, and the neighbor's width.
// - e5 vertical: covering planes beyond the nearest on each side.
// - e6 asymmetric: different spacings on the two sides against a mix of the
//   symmetric cases.
// - e7 interpolation: the lateral table's interpolation at off-grid points.
// - e8 vias: what a via adds to a wire, at its end and in its middle.
// - e9 nets: nets of a routed design, solved within a halo, against the
//   model applied to the same geometry by the reference evaluator.
// - e11 diagonal: a parallel wire on a nearby layer beside the wire rather
//   than over it, which the lateral and crossing tables do not see.
//
// bazelisk run starts it in its runfiles, where the default files are:
//   bazelisk run //src/pex/test:Experiments -- [--groups e1,e2,...]
//       [--itf FILE] [--layer NAME] [--threads N] [--tol X] [--csv FILE]
//       [--smoke]
// and for e9 the design and the model, built and saved when the file does
// not exist:
//       [--lef FILE]... [--def FILE | --odb FILE] [--model FILE]
//       [--nets a,b,... | --sample N] [--halo UM] [--margin UM]
//       [--depth N] [--model_tol X]
// Every row prints as it is measured; --csv also writes them as
// experiment,stack,layer,case,quantity,truth,sigma,model,error, where model
// is the simplification's value and error = model / truth - 1.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Reference.h"
#include "fsv/Geometry.h"
#include "fsv/Itf.h"
#include "fsv/NetWindow.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "odb/db.h"
#include "odb/defin.h"
#include "odb/lefin.h"
#include "pex/Model.h"
#include "pex/ModelBuilder.h"
#include "pex/ModelJson.h"
#include "pex/Patterns.h"
#include "utl/Logger.h"

namespace {

struct Options
{
  std::vector<std::string>
      groups{"e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "e11"};
  std::string itf = "src/fsv/test/sky130.itf";
  std::string layer = "met2";
  int threads = 1;
  double tol = 0.003;
  std::string csv;
  double extent = 10;
  bool smoke = false;
  // e9
  std::vector<std::string> lefs{
      "src/pex/test/sky130hd/sky130hd.tlef",
      "src/pex/test/sky130hd/sky130_fd_sc_hd_merged.lef"};
  std::string def = "src/fsv/test/field_solver_gcd.def";
  std::string odb;
  std::string model;
  std::vector<std::string> nets;
  int sample = 50;
  double halo = 10;
  double margin = 2;
  // The model's covering depth each way and its solver tolerance; 0 means
  // the run's.
  int depth = 99;
  double model_tol = 0;
  bool dump = false;  // print every box of each e9 window
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
    if (arg == "--groups") {
      o.groups = split(value());
    } else if (arg == "--itf") {
      o.itf = value();
    } else if (arg == "--layer") {
      o.layer = value();
    } else if (arg == "--threads") {
      o.threads = std::stoi(value());
    } else if (arg == "--tol") {
      o.tol = std::stod(value());
    } else if (arg == "--csv") {
      o.csv = value();
    } else if (arg == "--extent") {
      o.extent = std::stod(value());
    } else if (arg == "--smoke") {
      o.smoke = true;
    } else if (arg == "--lef") {
      if (!lefs_given) {
        o.lefs.clear();
        lefs_given = true;
      }
      o.lefs.push_back(value());
    } else if (arg == "--def") {
      o.def = value();
    } else if (arg == "--odb") {
      o.odb = value();
    } else if (arg == "--model") {
      o.model = value();
    } else if (arg == "--nets") {
      o.nets = split(value());
    } else if (arg == "--sample") {
      o.sample = std::stoi(value());
    } else if (arg == "--halo") {
      o.halo = std::stod(value());
    } else if (arg == "--margin") {
      o.margin = std::stod(value());
    } else if (arg == "--depth") {
      o.depth = std::stoi(value());
    } else if (arg == "--model_tol") {
      o.model_tol = std::stod(value());
    } else if (arg == "--dump") {
      o.dump = true;
    } else {
      throw std::invalid_argument("unknown argument " + std::string(arg));
    }
  }
  if (o.smoke) {
    o.tol = 0.05;
    o.halo = 3;
    o.margin = 1;
  }
  return o;
}

std::string join(const std::vector<std::string>& names, const char* sep = "+")
{
  std::string out;
  for (const std::string& n : names) {
    if (!out.empty()) {
      out += sep;
    }
    out += n;
  }
  return out;
}

// In a smoke run, only the first `keep` entries of a list.
template <class T>
std::vector<T> trim(std::vector<T> list, const bool smoke, const int keep = 1)
{
  if (smoke && static_cast<int>(list.size()) > keep) {
    list.resize(keep);
  }
  return list;
}

std::string fmt(const double x)
{
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.6g", x);
  return buf;
}

// One measurement: a quantity's truth, its standard error, and the value a
// simplification gives for it, when one applies.
struct Row
{
  std::string experiment;
  std::string name;
  std::string quantity;
  double truth;
  double sigma;
  std::optional<double> model;
};

class Lab
{
 public:
  explicit Lab(const Options& o)
      : o_(o),
        stack_(readStack(o.itf)),
        solver_({.threads = o.threads}),
        layer_(layer(o.layer))
  {
    if (!layer_.valid()) {
      throw std::invalid_argument("the experiments need a layer (--layer)");
    }
    for (const pex::Layer l : pex::layersBelow(stack_, layer_)) {
      below_.push_back(stack_.name(l));
    }
    for (const pex::Layer l : pex::layersAbove(stack_, layer_)) {
      above_.push_back(stack_.name(l));
    }
    if (!o.csv.empty()) {
      csv_.open(o.csv);
      if (!csv_.good()) {
        throw std::runtime_error("cannot write " + o.csv);
      }
      csv_ << "experiment,stack,layer,case,quantity,truth,sigma,model,error\n";
    }
  }

  // The stack, with whatever the reader skipped reported.
  static fsv::Stack readStack(const std::string& itf)
  {
    std::vector<fsv::ItfWarning> warnings;
    fsv::Stack stack = fsv::readItfFile(itf, &warnings);
    for (const fsv::ItfWarning& w : warnings) {
      std::printf("# %s line %d: %s\n", itf.c_str(), w.line, w.message.c_str());
    }
    return stack;
  }

  // The handle of a layer the experiments name; none for "". Throws for
  // a name the stack lacks.
  pex::Layer layer(const std::string& name) const
  {
    if (name.empty()) {
      return {};
    }
    const pex::Layer l = stack_.layer(name);
    if (!l.valid()) {
      throw std::invalid_argument("the stack has no layer " + name);
    }
    return l;
  }
  const fsv::Stack::Conductor& conductor(const std::string& name) const
  {
    return stack_.conductor(layer(name));
  }

  const fsv::Stack& stack() const { return stack_; }
  const Options& options() const { return o_; }
  pex::Layer layer() const { return layer_; }
  double wmin() const { return stack_.conductor(layer_).min_width; }
  double smin() const { return stack_.conductor(layer_).min_spacing; }
  // The covering layers, nearest first.
  const std::vector<std::string>& below() const { return below_; }
  const std::vector<std::string>& above() const { return above_; }
  std::string nearestBelow() const
  {
    return below_.empty() ? "" : below_.front();
  }
  std::string nearestAbove() const
  {
    return above_.empty() ? "" : above_.front();
  }

  fsv::CapRow solve(const fsv::Geometry& geometry,
                    const int master,
                    const double tol)
  {
    const fsv::Scene scene(geometry, stack_);
    return solver_.solve(scene, master, solveOptions(tol));
  }

  fsv::CapRow solve(const fsv::Geometry2D& geometry,
                    const int master,
                    const double tol)
  {
    const fsv::Scene2D scene(geometry, stack_);
    return solver_.solve(scene, master, solveOptions(tol));
  }

  void row(const std::string& experiment,
           const std::string& name,
           const std::string& quantity,
           const double truth,
           const double sigma,
           const std::optional<double> model = std::nullopt)
  {
    const Row r{experiment, name, quantity, truth, sigma, model};
    rows_.push_back(r);
    std::string error;
    if (model) {
      error = fmt(*model / truth - 1);
    }
    std::printf("%-4s %-40s %-12s %12s %10s %12s %10s\n",
                experiment.c_str(),
                name.c_str(),
                quantity.c_str(),
                fmt(truth).c_str(),
                fmt(sigma).c_str(),
                model ? fmt(*model).c_str() : "",
                error.c_str());
    std::fflush(stdout);
    if (csv_.is_open()) {
      csv_ << experiment << ',' << stack_.technology() << ',' << o_.layer << ','
           << name << ',' << quantity << ',' << fmt(truth) << ',' << fmt(sigma)
           << ',' << (model ? fmt(*model) : "") << ',' << error << '\n';
      csv_.flush();
    }
  }

 private:
  static fsv::SolveOptions solveOptions(const double tol)
  {
    fsv::SolveOptions options;
    options.tol_total = tol;
    options.tol_coupling = tol;
    return options;
  }

  Options o_;
  fsv::Stack stack_;
  fsv::Solver solver_;
  const pex::Layer layer_;
  std::vector<std::string> below_;
  std::vector<std::string> above_;
  std::vector<Row> rows_;
  std::ofstream csv_;
};

// A covering context: the plane below and above, "" for none.
struct Context
{
  std::string below;
  std::string above;

  std::string name() const
  {
    return (below.empty() ? "sub" : below) + "/"
           + (above.empty() ? "open" : above);
  }
};

std::vector<Context> contexts(const Lab& lab, const bool all)
{
  std::vector<Context> out{{"", ""}};
  if (all) {
    out.push_back({lab.nearestBelow(), ""});
    out.push_back({"", lab.nearestAbove()});
  }
  out.push_back({lab.nearestBelow(), lab.nearestAbove()});
  return out;
}

// Per-unit-length values of a wire alone in a context, from the 2D pattern:
// total and, with a plane above or below, the couplings to them.
struct PerLength
{
  double total;
  double below;
  double above;
  double ground() const { return total - below - above; }
};

PerLength perLength(Lab& lab,
                    const Context& ctx,
                    const double width,
                    const double tol)
{
  pex::LateralPattern p;
  p.layer = lab.layer();
  p.width = width;
  p.below = lab.layer(ctx.below);
  p.above = lab.layer(ctx.above);
  p.extent = lab.options().extent;
  const pex::LateralPattern::Section s = p.build(lab.stack());
  const fsv::CapRow row = lab.solve(s.geometry, s.wire, tol);
  return {row.total(),
          s.below != pex::kNullNetIndex ? row.coupling(s.below) : 0,
          s.above != pex::kNullNetIndex ? row.coupling(s.above) : 0};
}

// A 3D wire along y over [0, length], centered on x = 0, with the context's
// planes extending `margin` past its ends.
struct Wire3D
{
  pex::PatternBuilder builder;
  pex::NetIndex wire;
  pex::NetIndex below = pex::kNullNetIndex;
  pex::NetIndex above = pex::kNullNetIndex;
};

Wire3D wire3D(const Lab& lab,
              const Context& ctx,
              const double width,
              const double length,
              const double margin)
{
  pex::EndPattern pattern;
  pattern.layer = lab.layer();
  pattern.width = width;
  pattern.length = length;
  pattern.below = lab.layer(ctx.below);
  pattern.above = lab.layer(ctx.above);
  pattern.extent = lab.options().extent;
  pattern.margin = margin;
  pex::EndPattern::Wire wire = pattern.build(lab.stack());
  return {pex::PatternBuilder(lab.stack(), std::move(wire.geometry)),
          wire.wire,
          wire.below,
          wire.above};
}

// Per-length values of a wire in a context from the 2D pattern, with or
// without neighbors at minimum spacing on both sides: the total, the
// couplings to the planes, and to the left neighbor.
struct PerLength2
{
  double total = 0;
  double below = 0;
  double above = 0;
  double left = 0;
  double plane(const bool over) const { return over ? above : below; }
};

PerLength2 perLength2(Lab& lab,
                      const Context& ctx,
                      const bool neighbors,
                      const double tol)
{
  const Options& o = lab.options();
  pex::LateralPattern p;
  p.layer = lab.layer();
  p.width = lab.wmin();
  p.left = p.right = neighbors ? lab.smin() : -1;
  p.below = lab.layer(ctx.below);
  p.above = lab.layer(ctx.above);
  p.extent = o.extent;
  const pex::LateralPattern::Section s = p.build(lab.stack());
  const fsv::CapRow row = lab.solve(s.geometry, s.wire, tol);
  PerLength2 c;
  c.total = row.total();
  c.below = s.below != pex::kNullNetIndex ? row.coupling(s.below) : 0;
  c.above = s.above != pex::kNullNetIndex ? row.coupling(s.above) : 0;
  c.left = s.left != pex::kNullNetIndex ? row.coupling(s.left) : 0;
  return c;
}

// Per-length values of a wire of minimum width along y under (or over) an
// array of crossing wires of width `wc` at `pitch` on `cover`, from two
// lengths, which cancels the ends. Optionally with neighbors at minimum
// spacing on both sides, and with a plane on the nearest layer on the
// other side.
struct Crossed
{
  double total = 0;
  double cross = 0;
  double ground = 0;
  double left = 0;
  double other = 0;  // the plane on the other side
  double sigma = 0;
};

Crossed crossed(Lab& lab,
                const std::string& cover,
                const bool over,
                const double wc,
                const double pitch,
                const bool neighbors,
                const std::string& other)
{
  const Options& o = lab.options();
  const std::vector<double> wanted
      = o.smoke ? std::vector<double>{4, 8} : std::vector<double>{10, 30};
  // Whole pitches, differing by at least one.
  const int n_short
      = std::max(1, static_cast<int>(std::lround(wanted[0] / pitch)));
  const int n_long
      = std::max(n_short + 1, static_cast<int>(std::lround(wanted[1] / pitch)));
  Crossed sum[2];
  double len[2];
  for (int i = 0; i < 2; i++) {
    pex::CrossingPattern pattern;
    pattern.layer = lab.layer();
    pattern.cover = lab.layer(cover);
    pattern.width = lab.wmin();
    pattern.crossing_width = wc;
    pattern.pitch = pitch;
    pattern.length = (i == 0 ? n_short : n_long) * pitch;
    pattern.spacing = neighbors ? lab.smin() : -1;
    pattern.other = lab.layer(other);
    pattern.extent = o.extent;
    const pex::CrossingPattern::Array g = pattern.build(lab.stack());
    const fsv::CapRow row = lab.solve(g.geometry, g.wire, o.tol);
    sum[i].total = row.total();
    sum[i].cross = row.coupling(g.cross);
    sum[i].ground = -row.ground;
    sum[i].left = g.left != pex::kNullNetIndex ? row.coupling(g.left) : 0;
    sum[i].other = g.other != pex::kNullNetIndex ? row.coupling(g.other) : 0;
    sum[i].sigma = row.sigma[g.wire];
    len[i] = g.length;
  }
  const double dl = len[1] - len[0];
  Crossed c;
  c.total = (sum[1].total - sum[0].total) / dl;
  c.cross = (sum[1].cross - sum[0].cross) / dl;
  c.ground = (sum[1].ground - sum[0].ground) / dl;
  c.left = (sum[1].left - sum[0].left) / dl;
  c.other = (sum[1].other - sum[0].other) / dl;
  c.sigma = std::hypot(sum[0].sigma, sum[1].sigma) / dl;
  return c;
}

// e2: a wire of minimum width under (or over) an array of crossing wires on
// the nearest covering layer, at a coverage fraction f = width / pitch.
// Two simplifications: the plane × coverage rule (f times the plane's
// per-length value plus 1 - f times the open one), and a blend, where the
// array's share g of the plane's effect is measured on the wire alone and
// then applied to the wire with neighbors, with a plane on the other side,
// and with both: a blend table per (layer, cover, wc, pitch) that stays
// separate from the lateral table.
void crossing(Lab& lab)
{
  const Options& o = lab.options();
  for (const std::string& cover : {lab.nearestAbove(), lab.nearestBelow()}) {
    if (cover.empty()) {
      continue;
    }
    const bool over = cover == lab.nearestAbove();
    const std::string other = over ? lab.nearestBelow() : lab.nearestAbove();
    const Context plane{over ? "" : cover, over ? cover : ""};
    const Context with_other{over ? other : "", over ? "" : other};
    const Context both{over ? other : cover, over ? cover : other};
    const double cmin = lab.conductor(cover).min_width;
    // 2D values: [neighbors][other plane] for the open and the plane case.
    PerLength2 open[2][2];
    PerLength2 covered[2][2];
    for (const int n : {0, 1}) {
      for (const int p : {0, 1}) {
        if (p == 1 && other.empty()) {
          continue;
        }
        open[n][p]
            = perLength2(lab, p ? with_other : Context{"", ""}, n, o.tol / 3);
        covered[n][p] = perLength2(lab, p ? both : plane, n, o.tol / 3);
      }
    }
    for (const double wf : trim(std::vector<double>{1, 2, 5}, o.smoke)) {
      const double wc = wf * cmin;
      for (const double f :
           trim(std::vector<double>{0.1, 0.25, 0.5, 0.75}, o.smoke)) {
        const double pitch = wc / f;
        const Crossed base = crossed(lab, cover, over, wc, pitch, false, "");
        const std::string name = (over ? "over " : "under ") + cover
                                 + " wc=" + fmt(wf) + "wmin f=" + fmt(f);
        const PerLength2& o00 = open[0][0];
        const PerLength2& c00 = covered[0][0];
        lab.row("e2",
                name,
                "total/len",
                base.total,
                base.sigma,
                f * c00.total + (1 - f) * o00.total);
        lab.row("e2",
                name,
                "cross/len",
                base.cross,
                base.sigma,
                f * c00.plane(over));
        lab.row("e2",
                name,
                "ground/len",
                base.ground,
                base.sigma,
                f * (c00.total - c00.plane(over)) + (1 - f) * o00.total);
        // The array's share of the plane's effect, on the wire alone.
        const double g_total
            = (base.total - o00.total) / (c00.total - o00.total);
        const double g_cross = base.cross / c00.plane(over);
        lab.row("e2",
                name,
                "g_total",
                g_total,
                base.sigma / (c00.total - o00.total));
        lab.row("e2", name, "g_cross", g_cross, base.sigma / c00.plane(over));
        for (const int n : {1, 0}) {
          for (const int p : {0, 1}) {
            if (n == 0 && p == 0) {
              continue;
            }
            if (p == 1 && other.empty()) {
              continue;
            }
            const Crossed c
                = crossed(lab, cover, over, wc, pitch, n, p ? other : "");
            const PerLength2& op = open[n][p];
            const PerLength2& cp = covered[n][p];
            const std::string variant
                = name + (n ? " +neighbors" : "") + (p ? " +" + other : "");
            lab.row("e2",
                    variant,
                    "total/len",
                    c.total,
                    c.sigma,
                    (1 - g_total) * op.total + g_total * cp.total);
            lab.row("e2",
                    variant,
                    "cross/len",
                    c.cross,
                    c.sigma,
                    g_cross * cp.plane(over));
            if (n) {
              lab.row("e2",
                      variant,
                      "left/len",
                      c.left,
                      c.sigma,
                      (1 - g_total) * op.left + g_total * cp.left);
            }
            if (p) {
              lab.row(
                  "e2",
                  variant,
                  "other/len",
                  c.other,
                  c.sigma,
                  (1 - g_total) * op.plane(!over) + g_total * cp.plane(!over));
            }
          }
        }
      }
    }
  }
}

// e3: short wires against per-unit-length × length. The excess is what the
// two ends add; it should settle to a constant once the ends stop
// interacting. Then two line ends facing each other across a gap.
void ends(Lab& lab)
{
  const Options& o = lab.options();
  const std::vector<double> lengths
      = o.smoke ? std::vector<double>{0.5, 2}
                : std::vector<double>{0.3, 0.5, 1, 2, 5, 10};
  for (const Context& ctx : trim(contexts(lab, true), o.smoke)) {
    for (const double wf : trim(std::vector<double>{1, 2, 5}, o.smoke)) {
      const double w = wf * lab.wmin();
      const PerLength c = perLength(lab, ctx, w, o.tol / 3);
      for (const double length : lengths) {
        Wire3D g = wire3D(lab, ctx, w, length, o.extent);
        const fsv::CapRow row = lab.solve(g.builder.geometry(), g.wire, o.tol);
        const std::string name
            = ctx.name() + " w=" + fmt(wf) + "wmin L=" + fmt(length);
        lab.row("e3",
                name,
                "total",
                row.total(),
                row.sigma[g.wire],
                c.total * length);
        lab.row("e3",
                name,
                "end_excess",
                row.total() - c.total * length,
                row.sigma[g.wire]);
      }
    }
  }
  // Facing ends: wire a over [0, L], wire b over [L + gap, 2L + gap].
  const double w = lab.wmin();
  const double length = o.smoke ? 2 : 5;
  const Context ctx{"", ""};
  Wire3D alone = wire3D(lab, ctx, w, length, 0);
  const double single
      = lab.solve(alone.builder.geometry(), alone.wire, o.tol).total();
  for (const double gf : trim(std::vector<double>{1, 2, 5}, o.smoke)) {
    const double gap = gf * lab.smin();
    Wire3D g = wire3D(lab, ctx, w, length, 0);
    const pex::NetIndex b = g.builder.net("b");
    g.builder.add(
        lab.layer(), b, -w / 2, length + gap, w / 2, 2 * length + gap);
    const fsv::CapRow row = lab.solve(g.builder.geometry(), g.wire, o.tol);
    const std::string name = "facing gap=" + fmt(gf) + "smin L=" + fmt(length);
    lab.row("e3", name, "total", row.total(), row.sigma[g.wire], single);
    lab.row("e3", name, "end_coupling", row.coupling(b), row.sigma[b]);
  }
}

// A cross section with k neighbors on each side of a minimum-width wire,
// all at the same spacing, in a context.
struct Neighbors
{
  fsv::Geometry2D geometry;
  pex::NetIndex wire;
  std::vector<int> left;
  std::vector<int> right;
};

Neighbors neighbors(const Lab& lab,
                    const Context& ctx,
                    const int k,
                    const double spacing,
                    const double neighbor_width)
{
  const Options& o = lab.options();
  pex::SectionBuilder b(lab.stack());
  Neighbors n;
  const double half = lab.wmin() / 2;
  n.wire = b.net("wire");
  b.add(lab.layer(), n.wire, -half, half);
  for (int i = 1; i <= k; i++) {
    const double edge = half + i * spacing + (i - 1) * neighbor_width;
    n.left.push_back(b.net("l" + std::to_string(i)));
    b.add(lab.layer(), n.left.back(), -edge - neighbor_width, -edge);
    n.right.push_back(b.net("r" + std::to_string(i)));
    b.add(lab.layer(), n.right.back(), edge, edge + neighbor_width);
  }
  if (!ctx.below.empty()) {
    b.add(lab.layer(ctx.below), b.net("below"), -o.extent, o.extent);
  }
  if (!ctx.above.empty()) {
    b.add(lab.layer(ctx.above), b.net("above"), -o.extent, o.extent);
  }
  n.geometry = std::move(b.geometry());
  return n;
}

// e4: neighbors beyond the nearest, and the nearest neighbor's width.
// The simplification keeps only the nearest neighbor at minimum width.
void lateral(Lab& lab)
{
  const Options& o = lab.options();
  for (const Context& ctx : trim(contexts(lab, false), o.smoke)) {
    for (const double sf : trim(std::vector<double>{1, 2, 3}, o.smoke)) {
      const double s = sf * lab.smin();
      double total1 = 0;
      double c1 = 0;
      for (const int k : trim(std::vector<int>{1, 2, 3}, o.smoke, 2)) {
        const Neighbors n = neighbors(lab, ctx, k, s, lab.wmin());
        const fsv::CapRow row = lab.solve(n.geometry, n.wire, o.tol);
        const std::string name
            = ctx.name() + " s=" + fmt(sf) + "smin k=" + std::to_string(k);
        if (k == 1) {
          total1 = row.total();
          c1 = row.coupling(n.left[0]);
          lab.row("e4", name, "total", total1, row.sigma[n.wire]);
          lab.row("e4", name, "c1", c1, row.sigma[n.left[0]]);
        } else {
          lab.row("e4", name, "total", row.total(), row.sigma[n.wire], total1);
          lab.row("e4",
                  name,
                  "c1",
                  row.coupling(n.left[0]),
                  row.sigma[n.left[0]],
                  c1);
          lab.row("e4",
                  name,
                  "c" + std::to_string(k),
                  row.coupling(n.left[k - 1]),
                  row.sigma[n.left[k - 1]]);
        }
      }
    }
    double total_min = 0;
    double c_min = 0;
    for (const double wf : trim(std::vector<double>{1, 3, 10}, o.smoke, 2)) {
      const Neighbors n = neighbors(lab, ctx, 1, lab.smin(), wf * lab.wmin());
      const fsv::CapRow row = lab.solve(n.geometry, n.wire, o.tol);
      const std::string name = ctx.name() + " s=1smin wn=" + fmt(wf) + "wmin";
      if (wf == 1) {
        total_min = row.total();
        c_min = row.coupling(n.left[0]);
        lab.row("e4", name, "total", total_min, row.sigma[n.wire]);
        lab.row("e4", name, "c1", c_min, row.sigma[n.left[0]]);
      } else {
        lab.row("e4", name, "total", row.total(), row.sigma[n.wire], total_min);
        lab.row("e4",
                name,
                "c1",
                row.coupling(n.left[0]),
                row.sigma[n.left[0]],
                c_min);
      }
    }
  }
}

// e5: planes on the three nearest layers below and above, present or absent
// in every combination. The simplification keeps only the nearest present
// plane on each side.
void vertical(Lab& lab)
{
  const Options& o = lab.options();
  const std::vector<std::string> below = trim(lab.below(), o.smoke, 1);
  const std::vector<std::string> above = trim(lab.above(), o.smoke, 1);
  const int nb = std::min<int>(3, below.size());
  const int na = std::min<int>(3, above.size());
  std::map<std::string, double> nearest_only;
  auto run = [&](const std::vector<std::string>& b,
                 const std::vector<std::string>& a) {
    pex::SectionBuilder sb(lab.stack());
    const pex::NetIndex wire = sb.net("wire");
    sb.add(lab.layer(), wire, -lab.wmin() / 2, lab.wmin() / 2);
    for (const std::string& layer : b) {
      sb.add(lab.layer(layer), sb.net(layer), -o.extent, o.extent);
    }
    for (const std::string& layer : a) {
      sb.add(lab.layer(layer), sb.net(layer), -o.extent, o.extent);
    }
    const fsv::CapRow row = lab.solve(sb.geometry(), wire, o.tol);
    return std::pair{row.total(), row.sigma[wire]};
  };
  for (int mb = 0; mb < (1 << nb); mb++) {
    for (int ma = 0; ma < (1 << na); ma++) {
      std::vector<std::string> b;
      std::vector<std::string> a;
      for (int i = 0; i < nb; i++) {
        if (mb & (1 << i)) {
          b.push_back(below[i]);
        }
      }
      for (int i = 0; i < na; i++) {
        if (ma & (1 << i)) {
          a.push_back(above[i]);
        }
      }
      const std::string key = (b.empty() ? "sub" : b.front()) + "/"
                              + (a.empty() ? "open" : a.front());
      const auto [total, sigma] = run(b, a);
      const std::string name = (b.empty() ? "sub" : join(b)) + "/"
                               + (a.empty() ? "open" : join(a));
      const bool nearest = b.size() <= 1 && a.size() <= 1;
      if (nearest) {
        nearest_only[key] = total;
        lab.row("e5", name, "total", total, sigma);
      } else {
        lab.row("e5", name, "total", total, sigma, nearest_only.at(key));
      }
    }
  }
}

// The lateral tables of the layer, for e6 and e7: two contexts, the wire
// alone and between the nearest covers.
pex::Model buildTables(const Lab& lab)
{
  const Options& o = lab.options();
  pex::BuildOptions b;
  if (o.smoke) {
    b.width_factors = {1};
    b.spacing_factors = {1, 3};
  }
  b.below_depth = o.smoke ? 0 : 1;
  b.above_depth = o.smoke ? 0 : 1;
  b.tol = o.tol;
  b.threads = o.threads;
  b.extent = o.extent;
  const auto start = std::chrono::steady_clock::now();
  b.progress = [&](const int done, const int total) {
    if (done % 50 == 0 || done == total) {
      const double seconds = std::chrono::duration<double>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
      std::printf("# tables: %d of %d solves, %.0f s\n", done, total, seconds);
      std::fflush(stdout);
    }
  };
  pex::Model model;
  pex::buildLateral(lab.stack(), lab.layer(), b, model);
  return model;
}

std::string spacingName(const pex::LateralTable& t, const int s)
{
  return s == static_cast<int>(t.spacings().size()) ? "open"
                                                    : fmt(t.spacings()[s]);
}

// e6: unequal spacings against the mix of the two symmetric cases: the
// ground averaged, each coupling from its own side's symmetric case.
void asymmetric(Lab& lab, const pex::Model& model)
{
  const double tol = lab.options().tol;
  for (const auto& [key, t] : model.laterals()) {
    if (!key.below.empty() && key.above.empty()) {
      continue;  // the wire alone and between covers are enough
    }
    if (key.below.empty() != key.above.empty()) {
      continue;
    }
    const Context ctx{key.below, key.above};
    const int n = t.spacingCount();
    for (int w = 0; w < t.widthCount(); w++) {
      for (int l = 0; l < n; l++) {
        {
          // The grid itself, for offline checks of the interpolation.
          const pex::LateralCap& v = t.at(w, l, l);
          const std::string name = ctx.name() + " w=" + fmt(t.widths()[w])
                                   + " s=" + spacingName(t, l) + "/"
                                   + spacingName(t, l);
          const double total = v.total();
          lab.row("e6", name, "ground", v.rest(), tol * total);
          if (l < n - 1) {
            lab.row("e6", name, "left", v.left, tol * total);
          }
        }
        for (int r = l + 1; r < n; r++) {
          const pex::LateralCap& truth = t.at(w, l, r);
          const pex::LateralCap& ll = t.at(w, l, l);
          const pex::LateralCap& rr = t.at(w, r, r);
          const double total = truth.total();
          const std::string name = ctx.name() + " w=" + fmt(t.widths()[w])
                                   + " s=" + spacingName(t, l) + "/"
                                   + spacingName(t, r);
          lab.row("e6",
                  name,
                  "ground",
                  truth.rest(),
                  tol * total,
                  (ll.rest() + rr.rest()) / 2);
          if (l < n - 1) {
            lab.row("e6", name, "left", truth.left, tol * total, ll.left);
          }
          if (r < n - 1) {
            lab.row("e6", name, "right", truth.right, tol * total, rr.right);
          }
        }
      }
    }
  }
}

// e7: the interpolated table against solves at off-grid widths and
// spacings, including spacings past the grid, which the table treats as
// open.
void interpolation(Lab& lab, const pex::Model& model)
{
  const Options& o = lab.options();
  struct Pair
  {
    double left;  // multiples of the minimum spacing; < 0 is open
    double right;
  };
  const std::vector<Pair> pairs
      = o.smoke ? std::vector<Pair>{{1.12, 1.12}, {1.73, -1}}
                : std::vector<Pair>{{1.12, 1.12},
                                    {1.73, 1.73},
                                    {2.45, 2.45},
                                    {3.87, 3.87},
                                    {7.07, 7.07},
                                    {1.12, 3.87},
                                    {1.73, -1},
                                    {12, 12},
                                    {15, -1}};
  for (const auto& [key, t] : model.laterals()) {
    if (key.below.empty() != key.above.empty()) {
      continue;
    }
    const Context ctx{key.below, key.above};
    // 20 is past the grid: the table clamps it to its widest wire.
    for (const double wf :
         trim(std::vector<double>{1.22, 2.45, 7.07, 20}, o.smoke)) {
      const double w = wf * lab.wmin();
      for (const Pair& p : pairs) {
        const double sl = p.left < 0 ? -1 : p.left * lab.smin();
        const double sr = p.right < 0 ? -1 : p.right * lab.smin();
        pex::LateralPattern pattern;
        pattern.layer = lab.layer();
        pattern.width = w;
        pattern.left = sl;
        pattern.right = sr;
        pattern.below = lab.layer(key.below);
        pattern.above = lab.layer(key.above);
        pattern.extent = o.extent;
        const pex::LateralPattern::Section s = pattern.build(lab.stack());
        const fsv::CapRow row = lab.solve(s.geometry, s.wire, o.tol);
        const double left
            = s.left != pex::kNullNetIndex ? row.coupling(s.left) : 0;
        const double right
            = s.right != pex::kNullNetIndex ? row.coupling(s.right) : 0;
        const pex::LateralCap m = model.lateral(key, w, sl, sr);
        const std::string name
            = ctx.name() + " w=" + fmt(wf)
              + "wmin s=" + (p.left < 0 ? "open" : fmt(p.left)) + "/"
              + (p.right < 0 ? "open" : fmt(p.right)) + "smin";
        lab.row("e7",
                name,
                "ground",
                row.total() - left - right,
                row.sigma[s.wire],
                m.rest());
        if (s.left != pex::kNullNetIndex) {
          lab.row("e7", name, "left", left, row.sigma[s.left], m.left);
        }
        if (s.right != pex::kNullNetIndex) {
          lab.row("e7", name, "right", right, row.sigma[s.right], m.right);
        }
      }
    }
  }
}

// The end excess of a wire of minimum width in a context: its 3D total
// less per-unit-length × length, for a length past which the ends no
// longer interact.
double endExcess(Lab& lab, const Context& ctx, const double length)
{
  const Options& o = lab.options();
  const double w = lab.wmin();
  const PerLength c = perLength(lab, ctx, w, o.tol / 3);
  Wire3D g = wire3D(lab, ctx, w, length, o.extent);
  return lab.solve(g.builder.geometry(), g.wire, o.tol).total()
         - c.total * length;
}

// e1: a wire of minimum width whose context changes along it: a neighbor
// at minimum spacing beside its first third, and the nearest cover above
// its second half. The simplification sums the pieces' per-unit-length
// values and adds half the end excess of each end's context.
void additivity(Lab& lab)
{
  const Options& o = lab.options();
  if (lab.nearestAbove().empty()) {
    return;
  }
  const double w = lab.wmin();
  const double s = lab.smin();
  const double length = o.smoke ? 3 : 10;
  const Context open{"", ""};
  const Context covered{"", lab.nearestAbove()};
  // Per-unit-length values of the three pieces.
  pex::LateralPattern p;
  p.layer = lab.layer();
  p.width = w;
  p.left = s;
  p.extent = o.extent;
  const pex::LateralPattern::Section beside = p.build(lab.stack());
  const fsv::CapRow beside_row
      = lab.solve(beside.geometry, beside.wire, o.tol / 3);
  const PerLength alone = perLength(lab, open, w, o.tol / 3);
  const PerLength under = perLength(lab, covered, w, o.tol / 3);
  const double e_open = endExcess(lab, open, length);
  const double e_cover = endExcess(lab, covered, length);

  Wire3D g = wire3D(lab, open, w, length, 0);
  const pex::NetIndex neighbor = g.builder.net("neighbor");
  const double l1 = length / 3;
  const double l2 = length / 2;
  g.builder.add(lab.layer(), neighbor, -w / 2 - s - w, 0, -w / 2 - s, l1);
  const pex::NetIndex cover = g.builder.net("cover");
  g.builder.add(lab.layer(lab.nearestAbove()),
                cover,
                -o.extent,
                l2,
                o.extent,
                length + o.extent);
  const fsv::CapRow row = lab.solve(g.builder.geometry(), g.wire, o.tol);
  const std::string name
      = "neighbor 0-" + fmt(l1) + " cover " + fmt(l2) + "-" + fmt(length);
  lab.row("e1",
          name,
          "total",
          row.total(),
          row.sigma[g.wire],
          beside_row.total() * l1 + alone.total * (l2 - l1)
              + under.total * (length - l2) + e_open / 2 + e_cover / 2);
  lab.row("e1",
          name,
          "neighbor",
          row.coupling(neighbor),
          row.sigma[neighbor],
          beside_row.coupling(beside.left) * l1);
  lab.row("e1",
          name,
          "cover",
          row.coupling(cover),
          row.sigma[cover],
          under.above * (length - l2));
}

// e8: what a via adds to a wire's total: a wire on the layer with a via
// to the layer above (a square cut of the smaller minimum width) and a
// 1 µm perpendicular stub there, with and without the cut. The stub's share
// stands in as the simplification: the via's own capacitance is what the
// model would miss.
void vias(Lab& lab)
{
  const Options& o = lab.options();
  const std::string upper = lab.nearestAbove();
  if (upper.empty()) {
    return;
  }
  std::string cut;
  for (const fsv::Stack::Via& v : lab.stack().vias()) {
    if (v.lower == lab.layer() && v.upper == lab.layer(upper)) {
      cut = v.name;
    }
  }
  if (cut.empty()) {
    std::printf("# e8: no via from %s to %s\n", o.layer.c_str(), upper.c_str());
    return;
  }
  const double w = lab.wmin();
  const double side = std::min(w, lab.conductor(upper).min_width);
  const double length = o.smoke ? 1 : 2;
  for (const std::string& where :
       trim(std::vector<std::string>{"end", "middle"}, o.smoke)) {
    const double y = where == "end" ? length - side : (length - side) / 2;
    double totals[2];
    double sigma = 0;
    for (const bool with_cut : {false, true}) {
      Wire3D g = wire3D(lab, {"", ""}, w, length, 0);
      g.builder.add(lab.layer(upper),
                    g.wire,
                    -0.5 - side / 2,
                    y,
                    0.5 + side / 2,
                    y + side);
      if (with_cut) {
        g.builder.add(lab.layer(cut), g.wire, -side / 2, y, side / 2, y + side);
      }
      const fsv::CapRow row
          = lab.solve(g.builder.geometry(), g.wire, o.tol / 3);
      totals[with_cut] = row.total();
      sigma = std::hypot(sigma, row.sigma[g.wire]);
    }
    const std::string name = cut + " at the " + where + " L=" + fmt(length);
    lab.row("e8", name, "total", totals[1], sigma, totals[0]);
    lab.row("e8", name, "via_excess", totals[1] - totals[0], sigma);
  }
}

// The design for e9: LEF and DEF, or an ODB file.
struct Design
{
  std::unique_ptr<odb::dbDatabase, void (*)(odb::dbDatabase*)> db{
      nullptr,
      &odb::dbDatabase::destroy};
  odb::dbBlock* block = nullptr;
};

Design readDesign(const Options& o, utl::Logger& logger)
{
  Design d;
  d.db.reset(odb::dbDatabase::create());
  d.db->setLogger(&logger);
  if (!o.odb.empty()) {
    std::ifstream in(o.odb, std::ios::binary);
    if (!in.good()) {
      throw std::runtime_error("cannot read " + o.odb);
    }
    d.db->read(in);
    if (d.db->getChip() == nullptr || d.db->getChip()->getBlock() == nullptr) {
      throw std::runtime_error(o.odb + " has no block");
    }
    d.block = d.db->getChip()->getBlock();
    return d;
  }
  odb::lefin lef_reader(d.db.get(), &logger, false);
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
  odb::dbChip* chip = odb::dbChip::create(d.db.get(), tech);
  odb::defin def_reader(d.db.get(), &logger);
  if (!def_reader.readChip(libs, o.def.c_str(), chip)) {
    throw std::runtime_error("cannot read " + o.def);
  }
  d.block = chip->getBlock();
  return d;
}

// The model for e9: read from --model when the file exists and was built
// from this stack with these settings, else built for every routing layer
// of the design that the stack has, and saved there.
pex::Model netsModel(const Lab& lab, const Design& design)
{
  const Options& o = lab.options();
  pex::BuildOptions b;
  b.below_depth = b.above_depth = o.depth;
  if (o.smoke) {
    b.width_factors = {1};
    b.spacing_factors = {1};
    b.below_depth = b.above_depth = 0;
    b.end_length = 1;
    b.diagonal_width_factors = {1};
    b.diagonal_gap_factors = {0, 2};
    b.diagonal_spacing_factors = {1};
    b.diagonal_far_spacing_factors = {1};
    b.diagonal_depth = 1;
  }
  b.tol = o.model_tol > 0 ? o.model_tol : o.tol;
  b.threads = o.threads;
  b.extent = o.extent;
  // The technology's routing layers that the stack has.
  for (odb::dbTechLayer* layer : design.block->getTech()->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::ROUTING
        && lab.stack().isConductor(lab.stack().layer(layer->getName()))) {
      b.layers.push_back(layer->getName());
    }
  }
  const pex::ModelSource expected{
      lab.stack().technology(), o.itf, pex::buildFingerprint(lab.stack(), b)};
  if (!o.model.empty()) {
    std::ifstream in(o.model);
    if (in.good()) {
      pex::ModelSource source;
      pex::Model model = pex::readModel(in, &source);
      if (source.fingerprint != expected.fingerprint) {
        throw std::runtime_error(
            o.model + " was built from " + source.file + " ("
            + source.technology
            + ") with other settings, or the stack has changed since; "
              "delete it or name another file");
      }
      std::printf("# reading the model from %s\n", o.model.c_str());
      return model;
    }
  }
  const std::vector<std::string>& layers = b.layers;
  pex::Model model;
  const auto start = std::chrono::steady_clock::now();
  for (const std::string& layer : layers) {
    b.progress = [&](const int done, const int total) {
      if (done == total) {
        const double seconds = std::chrono::duration<double>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
        std::printf(
            "# %s: %d solves, %.0f s so far\n", layer.c_str(), total, seconds);
        std::fflush(stdout);
      }
    };
    pex::buildLayer(lab.stack(), lab.layer(layer), b, model);
  }
  if (!o.model.empty()) {
    pex::writeModelFile(model, expected, o.model);
    std::printf("# model written to %s\n", o.model.c_str());
  }
  return model;
}

// e9: nets of a routed design, each solved within a halo, against the
// model applied to the same boxes: the total, the ground (the plane and
// the grounded obstructions), the sum of the couplings to other nets, and
// every coupling above 1% of the total.
void nets(Lab& lab)
{
  const Options& o = lab.options();
  utl::Logger logger;
  const Design design = readDesign(o, logger);
  const pex::Model model = netsModel(lab, design);
  std::vector<odb::dbNet*> targets;
  if (!o.nets.empty()) {
    for (const std::string& name : o.nets) {
      odb::dbNet* net = design.block->findNet(name.c_str());
      if (net == nullptr) {
        throw std::invalid_argument("net " + name + " is not in the design");
      }
      targets.push_back(net);
    }
  } else {
    std::vector<odb::dbNet*> routed;
    for (odb::dbNet* net : design.block->getNets()) {
      if (!net->isSpecial() && net->getWire() != nullptr) {
        routed.push_back(net);
      }
    }
    const int want = o.smoke ? 1 : o.sample;
    const int step = std::max<int>(1, routed.size() / std::max(1, want));
    for (size_t i = 0;
         i < routed.size() && static_cast<int>(targets.size()) < want;
         i += step) {
      targets.push_back(routed[i]);
    }
  }
  const fsv::LayerExtent extent = [&](const std::string_view layer) {
    const fsv::Stack::ZExtent z
        = pex::layerExtent(lab.stack(), lab.layer(std::string(layer)));
    return std::make_pair(z.bottom, z.top);
  };
  for (odb::dbNet* net : targets) {
    const fsv::NetWindow window
        = fsv::readNetWindow(design.block, {net}, extent, {o.halo, o.margin});
    const fsv::Geometry& g = window.geometry;
    const pex::NetIndex target = window.targets.front();
    if (o.dump) {
      for (int b = 0; b < static_cast<int>(g.boxes().size()); b++) {
        const fsv::Box& box = g.boxes()[b];
        std::printf("#   box %-24s z %.3f-%.3f  x %.3f-%.3f  y %.3f-%.3f\n",
                    g.netName(g.boxNet(b)).c_str(),
                    box.lo[2],
                    box.hi[2],
                    box.lo[0],
                    box.hi[0],
                    box.lo[1],
                    box.hi[1]);
      }
    }
    const fsv::CapRow row = lab.solve(g, target, o.tol);
    const pex::NetEstimate est = pex::estimate(g, lab.stack(), model, target);
    const std::string name = net->getName();
    lab.row("e9", name, "total", row.total(), row.sigma[target], est.total());
    double grounded = est.ground;
    double est_coupled = 0;
    double coupled = 0;
    for (pex::NetIndex n = 0; n < g.netCount(); n++) {
      if (n == target) {
        continue;
      }
      const auto it = est.couplings.find(n);
      const double e = it == est.couplings.end() ? 0 : it->second;
      if (g.grounded(n)) {
        grounded += e;
        continue;
      }
      coupled += row.coupling(n);
      est_coupled += e;
      if (row.coupling(n) > 0.01 * row.total()) {
        lab.row(
            "e9", name, "c:" + g.netName(n), row.coupling(n), row.sigma[n], e);
      }
    }
    lab.row("e9", name, "ground", -row.ground, row.ground_sigma, grounded);
    lab.row("e9", name, "coupled", coupled, row.sigma[target], est_coupled);
    std::printf("#   %s: %d wires, %d pieces, %d ends, %d vias; leakage %.3g\n",
                name.c_str(),
                est.wires,
                est.pieces,
                est.ends,
                est.vias,
                row.window_leakage);
  }
}

// e11: a parallel wire on a layer below or above, laterally offset from the
// wire by an edge-to-edge gap (negative: overlapping by that much), alone
// and with neighbors at minimum spacing on the wire's own layer. The
// lateral tables see such a wire only as a cover when it overlaps half the
// width, so the coupling to it is what the model misses; the simplification
// is the wire's value without it.
void diagonal(Lab& lab)
{
  const Options& o = lab.options();
  const double w = lab.wmin();
  std::vector<std::string> others;
  if (!lab.below().empty()) {
    others.push_back(lab.below()[0]);
  }
  if (!lab.above().empty()) {
    others.push_back(lab.above()[0]);
  }
  if (lab.below().size() > 1) {
    others.push_back(lab.below()[1]);
  }
  for (const std::string& other : trim(others, o.smoke)) {
    const double omin = lab.conductor(other).min_width;
    // Alone, with neighbors on both sides, and with the far side open.
    for (const int variant : trim(std::vector<int>{0, 1, 2}, o.smoke)) {
      const bool neighbors = variant > 0;
      pex::DiagonalPattern pattern;
      pattern.layer = lab.layer();
      pattern.other = lab.layer(other);
      pattern.width = w;
      pattern.spacing = neighbors ? lab.smin() : -1;
      pattern.far_spacing = variant == 1 ? lab.smin() : -1;
      // The wire without the diagonal one.
      const pex::DiagonalPattern::Section base = pattern.build(lab.stack());
      const fsv::CapRow base_row
          = lab.solve(base.geometry, base.wire, o.tol / 3);
      for (const double wf : trim(std::vector<double>{1, 3, 10}, o.smoke)) {
        pattern.other_width = wf * omin;
        for (const double gap :
             trim(std::vector<double>{-w / 2, 0, 0.1, 0.2, 0.5, 1, 2, 4},
                  o.smoke,
                  2)) {
          pattern.gap = gap;
          const pex::DiagonalPattern::Section d = pattern.build(lab.stack());
          const fsv::CapRow row = lab.solve(d.geometry, d.wire, o.tol);
          const std::string name = other + " wd=" + fmt(wf)
                                   + "wmin gap=" + fmt(gap)
                                   + (variant == 1   ? " +neighbors"
                                      : variant == 2 ? " +neighbor near only"
                                                     : "");
          lab.row("e11",
                  name,
                  "diag/len",
                  row.coupling(d.diagonal),
                  row.sigma[d.diagonal]);
          lab.row("e11",
                  name,
                  "total/len",
                  row.total(),
                  row.sigma[d.wire],
                  base_row.total());
          if (neighbors) {
            lab.row("e11",
                    name,
                    "right/len",
                    row.coupling(d.right),
                    row.sigma[d.right],
                    base_row.coupling(base.right));
          }
        }
      }
    }
  }
}

bool wanted(const Options& o, const std::string& group)
{
  return std::find(o.groups.begin(), o.groups.end(), group) != o.groups.end();
}

}  // namespace

int main(int argc, char** argv)
{
  try {
    const Options o = parse(argc, argv);
    Lab lab(o);
    std::printf("# %s %s, tol %g, %d threads\n",
                lab.stack().technology().c_str(),
                o.layer.c_str(),
                o.tol,
                o.threads);
    std::printf("%-4s %-40s %-12s %12s %10s %12s %10s\n",
                "exp",
                "case",
                "quantity",
                "truth",
                "sigma",
                "model",
                "error");
    if (wanted(o, "e1")) {
      additivity(lab);
    }
    if (wanted(o, "e2")) {
      crossing(lab);
    }
    if (wanted(o, "e3")) {
      ends(lab);
    }
    if (wanted(o, "e4")) {
      lateral(lab);
    }
    if (wanted(o, "e5")) {
      vertical(lab);
    }
    if (wanted(o, "e6") || wanted(o, "e7")) {
      const pex::Model model = buildTables(lab);
      if (wanted(o, "e6")) {
        asymmetric(lab, model);
      }
      if (wanted(o, "e7")) {
        interpolation(lab, model);
      }
    }
    if (wanted(o, "e8")) {
      vias(lab);
    }
    if (wanted(o, "e9")) {
      nets(lab);
    }
    if (wanted(o, "e11")) {
      diagonal(lab);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
  return 0;
}
