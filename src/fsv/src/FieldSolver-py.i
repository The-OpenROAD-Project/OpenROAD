// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The field solver for "openroad -python" and the openroad package: the
// same session as the Tcl commands (Design.getFieldSolver()), with
// boxes and rectangles given as numbers, and solve() returning the rows as
// dicts keyed like the Tcl results.

#ifdef BAZEL
%module(package="src.fsv") fsv_py
#else
%module fsv_py
#endif

%{
#include <map>
#include <string>
#include <vector>

#include "fsv/FieldSolver.h"
#include "fsv/Geometry.h"
#include "fsv/NetWindow.h"
#include "fsv/Solver.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace {

PyObject* toDict(const fsv::NetCapacitance& result)
{
  const fsv::CapRow& row = result.row;
  PyObject* dict = PyDict_New();
  auto put = [](PyObject* d, const std::string& key, PyObject* value) {
    PyDict_SetItemString(d, key.c_str(), value);
    Py_DECREF(value);
  };
  PyObject* couplings = PyDict_New();
  PyObject* sigmas = PyDict_New();
  for (int net = 0; net < static_cast<int>(row.cap.size()); net++) {
    if (net != row.master && !result.grounded[net]) {
      put(couplings, result.nets[net], PyFloat_FromDouble(row.coupling(net)));
      put(sigmas, result.nets[net], PyFloat_FromDouble(row.sigma[net]));
    }
  }
  put(dict, "total", PyFloat_FromDouble(row.total()));
  put(dict, "total_sigma", PyFloat_FromDouble(row.sigma[row.master]));
  put(dict, "ground", PyFloat_FromDouble(-row.ground));
  put(dict, "ground_sigma", PyFloat_FromDouble(row.ground_sigma));
  put(dict, "couplings", couplings);
  put(dict, "coupling_sigmas", sigmas);
  put(dict, "walks", PyLong_FromLongLong(row.walks));
  put(dict, "hops", PyLong_FromLongLong(row.hops));
  put(dict, "seconds", PyFloat_FromDouble(result.seconds));
  put(dict, "cross_section", PyBool_FromLong(result.cross_section));
  put(dict, "window_leakage", PyFloat_FromDouble(row.window_leakage));
  return dict;
}

utl::Logger* logger()
{
  return ord::OpenRoad::openRoad()->getLogger();
}

}  // namespace
%}

%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include "../../Exception-py.i"

%template(FieldSolverNames) std::vector<std::string>;
%template(FieldSolverLayerMap) std::map<std::string, std::string>;

namespace fsv {

%nodefaultctor FieldSolver;
%nodefaultdtor FieldSolver;

class FieldSolver
{
 public:
  void readItf(const std::string& path);
  void setPermittivity(double eps);
  void clearGeometry();
  std::vector<std::string> defaultTargets() const;
};

%extend FieldSolver {
  // The nets, without the grounded net of obstructions that addNets makes.
  std::vector<std::string> netNames() const
  {
    std::vector<std::string> names;
    for (const std::string& name : $self->netNames()) {
      if (name != fsv::kObstructionNet) {
        names.push_back(name);
      }
    }
    return names;
  }

  // A box in um, given by its corners.
  void addBox(const std::string& net,
              double x0,
              double y0,
              double z0,
              double x1,
              double y1,
              double z1)
  {
    $self->addBox(net, {{x0, y0, z0}, {x1, y1, z1}});
  }

  // A box on a layer of the stack, by its rectangle in the x-y plane.
  void addLayerRect(const std::string& net,
                    const std::string& layer,
                    double x0,
                    double y0,
                    double x1,
                    double y1)
  {
    $self->addLayerRect(net, layer, {x0, y0, x1, y1});
  }

  // A cross-section rectangle in the x-z plane.
  void addRect(const std::string& net,
               double x0,
               double z0,
               double x1,
               double z1)
  {
    $self->addRect(net, {{x0, z0}, {x1, z1}});
  }

  // A cross-section rectangle on a layer of the stack, by its x span.
  void addLayerSpan(const std::string& net,
                    const std::string& layer,
                    double x0,
                    double x1)
  {
    $self->addLayerSpan(net, layer, x0, x1);
  }

  // addNets and solve, below, check their options; here a negative one
  // takes the default.
  void _addNets(const std::vector<std::string>& nets,
                double halo,
                double margin,
                const std::map<std::string, std::string>& layer_map)
  {
    $self->addNets(nets,
                   halo,
                   margin,
                   std::map<std::string, std::string, std::less<>>(
                       layer_map.begin(), layer_map.end()));
  }

  PyObject* _solve(const std::vector<std::string>& nets,
                   double tol_total,
                   double tol_coupling,
                   long long min_walks,
                   long long max_walks,
                   long long seed,
                   int panels)
  {
    fsv::SolverOptions solver_options;
    fsv::SolveOptions options;
    if (tol_total >= 0) {
      options.tol_total = tol_total;
    }
    if (tol_coupling >= 0) {
      options.tol_coupling = tol_coupling;
    }
    if (min_walks >= 0) {
      options.min_walks = min_walks;
    }
    if (max_walks >= 0) {
      options.max_walks = max_walks;
    }
    if (seed >= 0) {
      solver_options.seed = seed;
    }
    if (panels >= 0) {
      solver_options.panels = panels;
    }
    solver_options.threads = ord::OpenRoad::openRoad()->getThreadCount();
    const std::vector<std::string> targets
        = nets.empty() ? $self->defaultTargets() : nets;
    if (targets.empty()) {
      logger()->error(
          utl::FSV, 34, "No nets to solve; add boxes or rectangles first.");
    }
    PyObject* rows = PyDict_New();
    try {
      for (const std::string& net : targets) {
        PyObject* row = toDict($self->solve(net, solver_options, options));
        PyDict_SetItemString(rows, net.c_str(), row);
        Py_DECREF(row);
      }
    } catch (...) {
      Py_DECREF(rows);
      throw;
    }
    return rows;
  }

%pythoncode %{
    def addNets(self, nets, halo=None, margin=None, layer_map=None):
        """Replaces the conductors with nets of the loaded design (a name or
        a list of names) and their neighbours, as add_field_solver_nets. halo and margin are in um
        (None: the defaults, multiples of the nets' height); layer_map
        maps design layers to differently named ITF layers."""
        _check_distance("halo", halo)
        _check_distance("margin", margin)
        self._addNets(
            _net_list(nets),
            -1 if halo is None else halo,
            -1 if margin is None else margin,
            dict(layer_map or {}),
        )

    def solve(
        self,
        nets=(),
        tol_total=None,
        tol_coupling=None,
        min_walks=None,
        max_walks=None,
        seed=None,
        panels=None,
    ):
        """Solves the nets (a name or a list of names; the default targets if
        none are given), as
        solve_capacitance, and returns their rows by net name. An option
        left at None keeps its default."""
        _check_number("tol_total", tol_total, "greater than 0", lambda v: v > 0)
        _check_number(
            "tol_coupling", tol_coupling, "greater than 0", lambda v: v > 0
        )
        _check_integer("min_walks", min_walks, "at least 0", lambda v: v >= 0)
        _check_integer("max_walks", max_walks, "at least 1", lambda v: v >= 1)
        _check_integer("seed", seed, "from 0 to 2^63 - 1", lambda v: 0 <= v < 2**63)
        _check_integer(
            "panels",
            panels,
            "a positive multiple of 16",
            lambda v: v > 0 and v % 16 == 0,
        )
        options = [
            -1 if v is None else v
            for v in (
                tol_total,
                tol_coupling,
                min_walks,
                max_walks,
                seed,
                panels,
            )
        ]
        return self._solve(_net_list(nets), *options)
%}
}

}  // namespace fsv

%pythoncode %{
import math


def _check_number(name, value, what, ok):
    # A finite number: nan fails the comparisons, but inf would pass them.
    if value is None:
        return
    if (
        isinstance(value, bool)
        or not isinstance(value, (int, float))
        or not math.isfinite(value)
        or not ok(value)
    ):
        raise ValueError(f"{name} must be a number {what}; got {value!r}")


def _check_integer(name, value, what, ok):
    if value is None:
        return
    if isinstance(value, bool) or not isinstance(value, int) or not ok(value):
        raise ValueError(f"{name} must be an integer {what}; got {value!r}")


def _net_list(nets):
    # One name or several; a str is iterable, but its characters are not nets.
    return [nets] if isinstance(nets, str) else list(nets)


def _check_distance(name, value):
    _check_number(name, value, "of at least 0", lambda v: v >= 0)
%}
