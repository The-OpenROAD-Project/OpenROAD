// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Extraction for "openroad -python" and the openroad package: the same
// session as the Tcl commands (Design.getPex()), with the same defaults.

#ifdef BAZEL
%module(package="src.pex") pex_py
#else
%module pex_py
#endif

%{
#include <string>
#include <vector>

#include "ord/OpenRoad.hh"
#include "pex/Extractor.h"
#include "pex/Pex.h"
%}

%include <std_string.i>
%include <std_vector.i>
%include "../../Exception-py.i"

%template(PexNames) std::vector<std::string>;

namespace pex {

%nodefaultctor Pex;
%nodefaultdtor Pex;

class Pex
{
 public:
  void readModel(const std::string& itf, const std::string& model);
};

%extend Pex {
  void _buildModel(const std::string& itf,
                   const std::string& out,
                   const std::vector<std::string>& layers,
                   double tol)
  {
    $self->buildModel(
        itf, out, layers, tol, ord::OpenRoad::openRoad()->getThreadCount());
  }

  void _extract(double max_res,
                double coupling_threshold,
                double coupling_ratio)
  {
    pex::ExtractOptions options;
    options.max_res = max_res;
    options.coupling_threshold = coupling_threshold;
    options.coupling_ratio = coupling_ratio;
    options.threads = ord::OpenRoad::openRoad()->getThreadCount();
    $self->extract(options);
  }

  void _writeSpef(const std::string& path,
                  const std::vector<std::string>& nets)
  {
    $self->writeSpef(path, nets);
  }

%pythoncode %{
    def buildModel(self, itf, out, layers=(), tol=0.005):
        """Builds a model of the ITF stack into out, as build_pex_model:
        the given conductor layers, or the technology's routing layers."""
        _check_number("tol", tol, "greater than 0", lambda v: v > 0)
        self._buildModel(itf, out, _name_list(layers), tol)

    def extract(self, max_res=50, coupling_threshold=0.1, coupling_ratio=0.03):
        """Extracts the design's signal nets into ODB, as extract_pex."""
        for name, value in (
            ("max_res", max_res),
            ("coupling_threshold", coupling_threshold),
            ("coupling_ratio", coupling_ratio),
        ):
            _check_number(name, value, "of at least 0", lambda v: v >= 0)
        self._extract(max_res, coupling_threshold, coupling_ratio)

    def writeSpef(self, path, nets=()):
        """Writes the parasitics as SPEF, as write_pex_spef: the nets (a
        name or a list of names), or every extracted net."""
        self._writeSpef(path, _name_list(nets))
%}
}

}  // namespace pex

%pythoncode %{
import math


def _check_number(name, value, what, ok):
    # A finite number: nan fails the comparisons, but inf would pass them.
    if (
        isinstance(value, bool)
        or not isinstance(value, (int, float))
        or not math.isfinite(value)
        or not ok(value)
    ):
        raise ValueError(f"{name} must be a number {what}; got {value!r}")


def _name_list(names):
    # One name or several; a str is iterable, but its characters are not.
    return [names] if isinstance(names, str) else list(names)
%}
