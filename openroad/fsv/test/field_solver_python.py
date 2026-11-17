# The field solver from Python: the session the Tcl commands use, through
# Design.getFieldSolver(), with the same results.
from openroad import Design, Tech

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
design = Design(tech)
design.readDef("field_solver_nets.def")
# With only the documented import, as openroad imports every tool's
# module: a FieldSolver, not a bare SWIG pointer.
solver = design.getFieldSolver()
assert hasattr(solver, "solve")
import fsv  # noqa: E402

assert isinstance(solver, fsv.FieldSolver)
solver.readItf("sky130.itf")

# Nets of the design, as field_solver_nets.tcl reads them: the same row.
solver.addNets(["a"], halo=1, margin=0.2)
assert list(solver.defaultTargets()) == ["a"]
rows = solver.solve(min_walks=32768, max_walks=32768)
assert list(rows) == ["a"]
row = rows["a"]
assert list(row) == [
    "total",
    "total_sigma",
    "ground",
    "ground_sigma",
    "couplings",
    "coupling_sigmas",
    "walks",
    "hops",
    "seconds",
    "cross_section",
    "window_leakage",
]
assert row["walks"] == 32768 and not row["cross_section"]
assert row["couplings"]["b"] > row["couplings"]["VPWR"]
assert "<obstructions>" not in row["couplings"]
assert row["window_leakage"] > 0
# The grounded net of obstructions is not one of the nets.
assert "<obstructions>" not in solver.netNames() and "b" in solver.netNames()

# A layer map, as -layer_map takes it; one net may be given by name.
solver.addNets("a", halo=1, layer_map={"met2": "met2"})
assert list(solver.defaultTargets()) == ["a"]

# Shapes by layer: the stack gives their z extents. A met1 box over a
# met2 one, and a cross section of the pair.
solver.clearGeometry()
solver.addLayerRect("m1", "met1", 0, 0, 4, 0.14)
solver.addLayerRect("m2", "met2", 0, 0, 4, 0.14)
rows = solver.solve("m1", tol_total=0.02)
assert list(rows) == ["m1"] and rows["m1"]["couplings"]["m2"] > 0
solver.clearGeometry()
solver.addLayerSpan("m1", "met1", 0, 0.14)
solver.addLayerSpan("m2", "met2", 0, 0.14)
rows = solver.solve(["m1"], tol_total=0.02)
assert rows["m1"]["cross_section"] and rows["m1"]["couplings"]["m2"] > 0

# Boxes in a uniform dielectric: both nets by default, reciprocal.
solver.clearGeometry()
solver.setPermittivity(3.9)
solver.addBox("p", 0, 0, 0.5, 5, 0.2, 0.7)
solver.addBox("q", 0, 0.4, 0.5, 5, 0.6, 0.7)
assert list(solver.netNames()) == ["p", "q"]
rows = solver.solve(tol_total=0.01)
assert set(rows) == {"p", "q"}
c_pq = rows["p"]["couplings"]["q"]
c_qp = rows["q"]["couplings"]["p"]
sigma = (
    rows["p"]["coupling_sigmas"]["q"] ** 2 + rows["q"]["coupling_sigmas"]["p"] ** 2
) ** 0.5
assert abs(c_pq - c_qp) < 4 * sigma

# A cross section: per unit length.
solver.clearGeometry()
solver.addRect("r", 0, 0.5, 0.2, 0.7)
solver.addRect("s", 0.4, 0.5, 0.6, 0.7)
rows = solver.solve(["r"], tol_total=0.02)
assert rows["r"]["cross_section"]

# Options out of range raise ValueError before solving, negative ones
# included; None keeps a default.
for options in (
    {"tol_total": 0},
    {"tol_total": -0.01},
    {"tol_coupling": 0},
    {"min_walks": -5},
    {"max_walks": 0},
    {"seed": -1},
    {"panels": 20},
    {"max_walks": 1.5},
    {"tol_total": float("inf")},
    {"tol_coupling": float("nan")},
):
    try:
        solver.solve(**options)
        raise AssertionError(f"{options} was accepted")
    except ValueError:
        pass
for options in ({"halo": -1}, {"margin": -0.5}, {"halo": float("inf")}):
    try:
        solver.addNets(["a"], **options)
        raise AssertionError(f"{options} was accepted")
    except ValueError:
        pass
assert solver.solve(["r"], tol_total=None, max_walks=16384)["r"]["walks"] <= 16384
# Errors of the solver itself are reported as in Tcl and raise.
try:
    solver.solve(["nope"])
    raise AssertionError("an unknown net was solved")
except RuntimeError:
    pass
print("pass")
