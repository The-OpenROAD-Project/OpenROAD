# Extraction from Python: the session the Tcl commands use, through
# Design.getPex(), as pex_extract.tcl does.
import os

from openroad import Design, Tech

tech = Tech()
tech.readLef("sky130hd/sky130hd.tlef")
tech.readLef("sky130hd/sky130_fd_sc_hd_merged.lef")
design = Design(tech)
design.readDef("../../fsv/test/field_solver_gcd.def")
extractor = design.getPex()
import pex  # noqa: E402

assert isinstance(extractor, pex.Pex)
extractor.readModel("../../fsv/test/sky130.itf", "sky130_coarse.json")
extractor.extract()

results = os.environ.get("RESULTS_DIR", "results")
os.makedirs(results, exist_ok=True)


def written_nets(path):
    with open(path) as f:
        return sum(1 for line in f if line.startswith("*D_NET"))


spef = os.path.join(results, "pex_python.spef")
extractor.writeSpef(spef)
assert written_nets(spef) == 122
extractor.writeSpef(spef, "_042_")
assert written_nets(spef) == 1

for bad in (-1, float("nan"), True, "1"):
    try:
        extractor.extract(max_res=bad)
    except ValueError as e:
        print(e)
    else:
        raise AssertionError(f"max_res={bad!r} accepted")
for bad in (-0.1, 1.5):
    try:
        extractor.extract(coupling_ratio=bad)
    except ValueError as e:
        print(e)
    else:
        raise AssertionError(f"coupling_ratio={bad!r} accepted")
extractor.compareFsv("_042_", halo=4, tol=0.02)
for kwargs in ({}, {"nets": "_042_", "sample": 2}, {"sample": 0}):
    try:
        extractor.compareFsv(**kwargs)
    except ValueError as e:
        print(e)
    else:
        raise AssertionError(f"compareFsv({kwargs}) accepted")
extractor.extract(coupling_threshold=1, coupling_ratio=1)
extractor.extract(coupling_to_ground=True)
try:
    extractor.extract(coupling_to_ground=1)
except ValueError as e:
    print(e)
else:
    raise AssertionError("coupling_to_ground=1 accepted")
print("pass")
