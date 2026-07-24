#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "dcalc/DmpCeff.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Sta.hh"
#include "sta/TableModel.hh"
#include "sta/TimingArc.hh"

using sta::DmpPi;
using sta::GateTableModel;
using sta::initSta;
using sta::LibertyCell;
using sta::LibertyLibrary;
using sta::MinMaxAll;
using sta::Pvt;
using sta::RiseFall;
using sta::Sta;
using sta::TimingArc;
using sta::TimingArcSeq;
using sta::TimingArcSet;

int main(int argc, char** argv)
{
  initSta();
  Sta* sta = new Sta;
  Sta::setSta(sta);
  sta->makeComponents();

  // 2. Load standard synthetic linear library
  std::string lib_path;
  if (argc > 1) {
    lib_path = argv[1];
  }
  LibertyLibrary* library = sta->readLiberty(lib_path.c_str(),
                                             sta->cmdScene(),
                                             MinMaxAll::max(),
                                             /* infer_latches */ false);
  if (!library) {
    std::cerr << "Error loading synthetic_linear.lib\n";
    return 1;
  }

  // 3. Find cell and timing model
  LibertyCell* cell = library->findLibertyCell("perfect_buffer");
  if (!cell) {
    std::cerr << "Error: Cell perfect_buffer not found in library\n";
    return 1;
  }
  TimingArc* arc = nullptr;
  for (TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->arcCount() > 0) {
      arc = arc_set->arcs()[0];
      break;
    }
  }
  if (!arc) {
    std::cerr << "Error: Cell has no timing arcs\n";
    return 1;
  }
  GateTableModel* gate_model = arc->gateTableModel();
  if (!gate_model) {
    std::cerr << "Error: Timing arc has no GateTableModel\n";
    return 1;
  }
  Pvt pvt(1.0f, 1.0f, 25.0f);
  const RiseFall* rf = RiseFall::rise();

  // 4. Anchor constants
  const double R_A = 2000.0;  // 2000 Ohm
  const double C_A = 2e-15;   // 2.0 fF

  // 5. Sweep loops (dimensionless parameters)
  // High density for <0.001% Trilinear LUT
  std::vector<double> xs;
  for (double val = 0.001; val <= 1000.0; val *= 1.41421356) {
    xs.push_back(val);
  }

  std::vector<double> ys;
  for (double val = 0.025; val < 1.0; val += 0.025) {
    ys.push_back(val);
  }

  std::vector<double> zs;
  for (double val = 0.1; val <= 10.0; val *= 1.34164079) {
    zs.push_back(val);
  }

  std::cout << "x_Rratio,y_Cratio,z_Tratio,k_shield\n";

  for (double x : xs) {
    for (double y : ys) {
      for (double z : zs) {
        double rd = R_A;
        double rpi = x * R_A;
        double c_tot = C_A;
        double c2 = y * c_tot;
        double c1 = c_tot - c2;
        double in_slew = z * R_A * c_tot;

        // Instantiate OpenSTA's actual DmpPi solver directly
        DmpPi dmp_pi(sta);
        dmp_pi.init(
            library, cell, &pvt, gate_model, rf, rd, in_slew, c2, rpi, c1);
        dmp_pi.gateDelaySlew();
        double ceff = dmp_pi.ceff();

        double k_shield = (ceff - c2) / c1;
        // Clamp to physical bounds
        k_shield = std::clamp(k_shield, 0.0, 1.0);
        std::cout << x << "," << y << "," << z << "," << k_shield << "\n";
      }
    }
  }

  // Exit cleanly bypassing static destructor cleanup issues
  std::exit(0);
}
