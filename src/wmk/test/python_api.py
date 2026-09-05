# The Python surface can do the same work as the Tcl one.
#
# Every keyed method takes a 32-byte key.  Nothing in Python builds a
# std::array, so without a conversion these methods are present and uncallable
# -- the module imports, the class appears, and every call raises TypeError.
# That is a worse state than not offering them at all, and it is invisible to
# any test that only checks the Tcl commands.
#
# So this embeds a mark, tags nets and reads a mark back, all from Python, with
# the key given both ways a caller would naturally have it: as the hex string
# the Tcl commands take, and as raw bytes.  It also checks that a key which is
# not a key is refused rather than quietly padded or truncated into one.

from openroad import Tech, Design
import helpers
import wmk

KEY_HEX = "0011223344556677889900aabbccddeeff00112233445566778899aabbccddee"

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
design = Design(tech)
design.readDef("gcd_placed.def")
watermark = design.getWatermark()

options = wmk.PlacementOptions()
options.hpwl_eps_um = 1.0
options.pair_dist_um = 3.0
options.pairs_per_tile = 64
# The slack screen is switched off deliberately.  What is being tested is that
# the bindings carry a key across into C++ and bring an answer back, and how
# many pairs a design yields under timing pressure is place.tcl's business.
# Leaving it on would make these counts depend on how the timing happened to be
# set up, which differs between the two build systems and is not what this test
# is for.
options.slack_threshold_ns = 0.0
options.post_guard = False

claims = helpers.make_result_file("python_api.csv")
committed = watermark.placementWatermark(KEY_HEX, options, claims)
print("committed from a hex key:", committed)

# The same key as raw bytes has to reach the same 24 pairs.  They are already
# in this key's order by now, so the second run simply finds them there.
again = watermark.placementWatermark(
    bytes.fromhex(KEY_HEX), options, helpers.make_result_file("python_api_bytes.csv")
)
print("committed from a bytes key:", again)

result = watermark.verifyPlacement(claims)
print("verification:", result.held, "of", result.checked)

tagged = watermark.selectNetsKeyed(KEY_HEX, 0.10)
print("nets tagged:", tagged)

for bad in ("abc", b"\x00" * 5, 42):
    try:
        watermark.selectNetsKeyed(bad, 0.10)
        print("ERROR: accepted", repr(bad), "as a key")
    except (TypeError, ValueError) as exception:
        print("refused", repr(bad), "->", type(exception).__name__)

ok = (
    committed == 24
    and again == 24
    and result.held == 24
    and result.checked == 24
    and tagged == 35
)
print("pass" if ok else "FAIL")
