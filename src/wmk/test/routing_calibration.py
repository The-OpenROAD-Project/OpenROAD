# Exhaust the four-net null distribution and check the public routing verdict.
# With one marked net and one draw, the unadjusted minimum accepted 9/16
# equally likely outcomes at alpha=0.5. A valid p-value may accept at most 8.
from pathlib import Path

from openroad import Tech, Design
import helpers
import odb
import wmk

# These names seed the sampler's first draw at sorted indices 2, 0, 3 and 1.
DESIGNS = ["wmk_null_0", "wmk_null_1", "wmk_null_2", "wmk_null_4"]
# Each key selects only the corresponding probe net at fraction 0.25.
KEYS = [
    "0000000000000000000000000000000000000000000000000000000000000006",
    "0000000000000000000000000000000000000000000000000000000000000001",
    "000000000000000000000000000000000000000000000000000000000000000d",
    "0000000000000000000000000000000000000000000000000000000000000007",
]

pvalues = []
sampled_passes = 0
exact_passes = 0
for name in DESIGNS:
    tech = Tech()
    tech.readLef("Nangate45/Nangate45.lef")
    design = Design(tech)
    fixture = Path(helpers.make_result_file(name + ".def"))
    fixture.write_text(
        'VERSION 5.8 ;\nDIVIDERCHAR "/" ;\nBUSBITCHARS "[]" ;\n'
        f"DESIGN {name} ;\nUNITS DISTANCE MICRONS 2000 ;\n"
        "DIEAREA ( 0 0 ) ( 4000 4000 ) ;\nEND DESIGN\n"
    )
    design.readDef(str(fixture))
    block = design.getBlock()
    layer = tech.getDB().getTech().findLayer("metal1")
    encoder = odb.dbWireEncoder()
    for i in range(4):
        net = odb.dbNet_create(block, "probe_" + str(i))
        encoder.begin(odb.dbWire_create(net))
        encoder.newPath(layer, "ROUTED")
        encoder.addPoint(0, 0)
        encoder.addPoint(3000 - i * 1000, 0)
        encoder.addPoint(3000 - i * 1000, i * 1000)
        encoder.end()

    for i, key in enumerate(KEYS):
        stat = design.getWatermark().verifyRouting(key, 0.25, 1)
        assert stat.eligible == 4 and stat.marked == 1
        assert abs(10**stat.log10_tail - (i + 1) / 4) < 1e-12
        sampled_passes += stat.p_r <= 0.5
        exact_passes += 10**stat.log10_tail <= 0.5
        pvalues.append(stat.pValue())
        for alpha in (0.25, 0.5, 0.75):
            verdict = design.evalTclString(
                "verify_watermark -routing_key_hex "
                + key
                + " -routing_fraction 0.25 -routing_permutations 1"
                + " -routing_alpha "
                + str(alpha)
                + " -min_stages 1"
            )
            assert int(verdict) == int(stat.pValue() <= alpha)

# Check every attainable threshold, including ties, rather than one alpha.
assert sampled_passes == 6 and exact_passes == 8
for count, pvalue in enumerate(sorted(pvalues), start=1):
    assert count / len(pvalues) <= pvalue + 1e-12
print("pass")
