# C++ validation must protect Python callers as well as Tcl wrappers.
from pathlib import Path

from openroad import Tech, Design
import helpers
import odb
import wmk

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
design = Design(tech)
design.readDef("gcd_placed.def")
watermark = design.getWatermark()
block = design.getBlock()
key = bytes(32)
claims = Path(helpers.make_result_file("numeric_options.csv"))
claims.write_text("existing claims\n")


def snapshot():
    return [
        (
            inst.getName(),
            inst.getLocation(),
            str(inst.getOrient()),
            str(inst.getPlacementStatus()),
        )
        for inst in block.getInsts()
    ]


def tags():
    return [
        net.getName()
        for net in block.getNets()
        if odb.dbBoolProperty.find(net, "watermark")
    ]


watermark.selectNetsKeyed(key, 0.25)
before, marked = snapshot(), tags()
assert marked


def refused(call):
    try:
        call()
    except RuntimeError:
        pass
    else:
        raise AssertionError("invalid numeric option was accepted")
    assert snapshot() == before
    assert tags() == marked
    assert claims.read_text() == "existing claims\n"


for option in ("pair_dist_um", "hpwl_eps_um", "slack_threshold_ns", "guard_degrade_ns"):
    for value in (-1.0, float("nan"), float("inf"), float("-inf"), 1e100):
        opts = wmk.PlacementOptions()
        setattr(opts, option, value)
        refused(lambda: watermark.placementWatermark(key, opts, str(claims)))
for option, values in {
    "grid_nx": (0, -1),
    "grid_ny": (0, -1),
    "pairs_per_tile": (-1,),
    "min_pairs_total": (-1,),
    "max_disp_um": (-1, 2147483647),
}.items():
    for value in values:
        opts = wmk.PlacementOptions()
        setattr(opts, option, value)
        refused(lambda: watermark.placementWatermark(key, opts, str(claims)))
for option in (
    "sibling_dist_um",
    "skew_margin_ns",
    "slew_headroom_frac",
    "cap_headroom_frac",
):
    for value in (-1.0, float("nan"), float("inf"), float("-inf"), 1e100):
        opts = wmk.CtsOptions()
        setattr(opts, option, value)
        refused(lambda: watermark.ctsWatermark(key, opts, str(claims)))
opts = wmk.CtsOptions()
opts.num_pairs = -1
refused(lambda: watermark.ctsWatermark(key, opts, str(claims)))
for fraction in (float("nan"), float("inf"), float("-inf"), 0.0, -1.0, 1.1):
    refused(lambda: watermark.selectNetsKeyed(key, fraction))
    refused(lambda: watermark.verifyRouting(key, fraction, 1))
    refused(lambda: watermark.reportWatermark(fraction))
for draws in (0, -1):
    refused(lambda: watermark.verifyRouting(key, 0.25, draws))

# Conversion boundary and relaxed HPWL arithmetic: these valid distances
# approach INT_MAX DBU, so doubling the relaxed HPWL budget requires int64.
opts = wmk.PlacementOptions()
opts.pair_dist_um = 2147483646 / block.getDbUnitsPerMicron()
opts.hpwl_eps_um = opts.pair_dist_um
opts.max_disp_um = 2147483647 // block.getDbUnitsPerMicron()
opts.pairs_per_tile = 0
opts.slack_threshold_ns = 0
opts.post_guard = False
assert watermark.placementWatermark(key, opts, str(claims)) == 0
assert snapshot() == before
print("pass")
