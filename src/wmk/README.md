# Watermark

The watermark module (`wmk`) embeds keyed ownership evidence in a
physical design and checks it back. It implements PDMarks, a Kerckhoffs-compliant watermarking
scheme whose security rests on a secret key rather than on hidden construction:
the algorithms are public and only the key is not. Three stages carry marks:

-   Placement: a keyed subset of same-row, same-width cell pairs is put into a
    keyed left-to-right order
-   Clock tree: a keyed subset of leaf clock buffers (LCB) is driven to a keyed
    sequential-fanout parity
-   Routing: a keyed subset of signal nets is routed under an inflated cost for
    wrong-way wiring, and detected as a population effect

The key selects both which objects are marked and what value each carries, so an
observer who knows the algorithm still cannot locate the marks. Verification
needs no flow and no re-run: it reads a claim file, or for routing the key alone,
against any loaded database.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Generate Watermark Key

The `generate_watermark_key` command draws a secret key and derives the three stage
keys from it. It returns a dictionary with the entries `key_hex`, `nonce_hex`,
`design_id`, `placement`, `cts` and `routing`.

A stage key is `HMAC-SHA256(key, design_id, nonce, "stage=" || stage)`.

The nonce identifies one watermark instance/run. The same secret key (`key`) and design
identifier (`design_id`) with a different nonce mark different objects with different values,
so one secret key can mark several copies of a design distinguishably, and can
mark it again after a revision without repeating the previous marks. The design
identifier and the nonce are public and belong with the design's records; only
the secret key does not. All three are needed again to verify, so `-public_file`
writes the two public ones on their own, leaving `-file` as the only thing that
has to be kept secret.

The secret key is never logged.
When using `-file`, the key is written with owner-only permissions. Existing private files with group or other permissions are rejected without modification. Private and public destinations must refer to different files, including through symbolic or hard links. Both outputs are validated and fully written to temporary files before either destination is replaced; each replacement is an atomic rename. If the system random source cannot be accessed, the command fails rather than falling back to a predictable source.

```tcl
generate_watermark_key
    -design_id design_id
    [-file file]
    [-key_hex key_hex]
    [-nonce_hex nonce_hex]
    [-public_file public_file]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-design_id` | Identifier of the design version being marked (e.g., `jpeg_NG45_v1`). |
| `-file` | Write the secret key, nonce and stage keys to this path, readable only by its owner. |
| `-key_hex` | Use this 64-character hex secret key instead of drawing one. |
| `-nonce_hex` | Use the specified nonce instead of generating a random nonce. The nonce must be provided as an even-length hexadecimal string. Automatically generated nonces are 16 bytes (128 bits). |
| `-public_file` | Write the design identifier and the nonce to this path. These are the public inputs to key derivation and carry no key material, so this file can be kept with the design's records and passed on. |

### Derive Watermark Key

The `derive_watermark_key` command re-derives one stage key and returns it as a
64-character hex string. Verification runs in a later process than embedding, so
this recovers the keys that were used without any of them having been stored.

```tcl
derive_watermark_key
    -design_id design_id
    -key_hex key_hex
    -nonce_hex nonce_hex
    -stage stage
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-design_id` | The identifier the keys were generated with. |
| `-key_hex` | The 64-character hex secret key. |
| `-nonce_hex` | The nonce the keys were generated with. |
| `-stage` | `placement`, `cts` or `routing`. |

### Place Watermark

The `place_watermark` command puts a keyed subset of cell pairs into a keyed
left-to-right order and writes the claimed pairs to `-claims_file`. It returns
the number of pairs claimed. Run it after detailed placement.

Only cells in the same row with the same width are paired, so a swap leaves the
row legal and the area unchanged. Candidates are screened on slack and on the
wirelength a swap would cost; the key then orders what survives and takes a
greedy non-overlapping prefix. The design is re-legalized afterwards, and a pair
whose cells then lost more than `-guard_degrade_ns` of slack is put back.

After any rollback and legalization, parasitics are re-estimated and the final
placement is checked against the original constrained endpoint setup and hold
slacks in every analysis scene. If selective rollback cannot meet the budget,
the entire original placement is restored. A requested guard without usable
constraints and signal wire RC produces a warning; Liberty alone does not
activate it. With timing available, the slack screen excludes unconstrained
candidate cells.

Every pair chosen is claimed, including pairs restored by the timing guard and
any whose mark did not survive legalization. Removing failed pairs after
observing the design would artificially inflate the extraction rate, in which
case it would be 1.0 on every design.

```tcl
place_watermark
    -claims_file file
    -key_hex key_hex
    [-grid_nx n]
    [-grid_ny n]
    [-guard_degrade_ns ns]
    [-hpwl_eps_um eps]
    [-max_disp_um disp]
    [-min_pairs_total n]
    [-pair_dist_um dist]
    [-pairs_per_tile n]
    [-slack_threshold_ns slack]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-claims_file` | Where to write the claims. |
| `-key_hex` | 64-character hex placement key. |
| `-grid_nx`, `-grid_ny` | Tiles across and down the core, so marks are spread rather than clustered. Both default to `8`. |
| `-guard_degrade_ns` | Slack a pair may cost before it is put back. Defaults to `0.02`. `0` disables the check. |
| `-hpwl_eps_um` | Largest half-perimeter wirelength change a swap may cost, in microns. Defaults to `0.05`. |
| `-max_disp_um` | Displacement allowed during re-legalization, in microns. Defaults to `5`. |
| `-min_pairs_total` | Below this many pairs, search again with a wider neighbourhood and twice the wirelength budget. Defaults to `64`. |
| `-pair_dist_um` | How far along the row to look for a partner, in microns. Defaults to `1.0`. |
| `-pairs_per_tile` | Most pairs to claim per tile. Defaults to `4`. |
| `-slack_threshold_ns` | Cells with less slack are left alone. Defaults to `0.20`. `0` disables the screen. |

### CTS Watermark

The `cts_watermark` command drives a keyed subset of leaf clock buffers (LCBs) to a
keyed sequential-fanout parity and writes the claimed pairs to `-claims_file`.
It returns the number of pairs claimed. Run it after clock tree synthesis on a
flat design, linked without `-hier`. Hierarchical CTS embedding is rejected before
any edits because moving a sink also requires updating module ports and nets.
CTS verification remains available for hierarchical designs.

LCBs are marked in pairs. Parity is changed by moving one flip-flop's clock
pin from one LCB of the pair to the other. A move is undone if it
increases clock latency spread or degrades a constrained endpoint's setup/hold
slack by more than `-skew_margin_ns`, or if it leaves
the LCB with less slew or capacitance headroom than the liberty cell allows.
Protected source/destination nets and fixed or protected sink instances are not
modified. A connection, extraction or timing error restores the trial sink's
original net and refreshes both nets' parasitics before propagating the error.

Pairs must have identical, nonempty clock sets in the active timing modes,
including mode identity, and provably equivalent clock logic. The command traces only unconditional Liberty
buffers and inverters, requires a common source net and matching inversion
parity, and stops at gates, muxes and sequential cells. Case analysis does not
remove these boundaries. Floating nets, multiple drivers and cycles are rejected.
Leaf carriers must be Liberty buffers or inverters; STA identifies sequential
clock pins without relying on LEF flags or pin names.

The skew guard measures the latest minus earliest propagated clock latency at
sequential clock pins, separately for each clock, analysis scene and source
edge. It compares against the original design throughout embedding; the budget
does not reset after each accepted move. This conservative latency spread is
different from the path-based `report_clock_skew` metric, which considers data
path relationships, uncertainty and common-path pessimism removal. A separate
STA guard checks every originally constrained endpoint's setup and hold slack,
for each scene and transition, against its fixed baseline with the same budget.
This catches path degradation even when the overall latency spread is unchanged.

Read Liberty and constraints, set signal and clock wire RC, estimate parasitics,
and propagate clocks before embedding. A pair without measurable clock timing is left unchanged and the
command reports the unavailable guard. It remains a claim if selected.

```tcl
cts_watermark
    -claims_file file
    -key_hex key_hex
    [-cap_headroom_frac frac]
    [-num_pairs n]
    [-sibling_dist_um dist]
    [-skew_margin_ns margin]
    [-slew_headroom_frac frac]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-claims_file` | Where to write the claims. |
| `-key_hex` | 64-character hex clock-tree key. |
| `-cap_headroom_frac` | Fraction of the liberty capacitance limit left unused. Defaults to `0.20`. |
| `-num_pairs` | Most pairs to mark, one bit each. Defaults to `32`. |
| `-sibling_dist_um` | Largest distance between the two buffers of a pair, in microns. Defaults to `20.0`. |
| `-skew_margin_ns` | Maximum clock latency-spread increase and endpoint slack degradation, in ns. Defaults to `0.020`. `0` permits no degradation. |
| `-slew_headroom_frac` | Fraction of the liberty slew limit left unused. Defaults to `0.20`. |

The slew reserve is checked for each scene and rise/fall edge; capacitance
reserve is checked in each scene. Limits include applicable SDC constraints and
Liberty limits. Both headroom fractions must be finite and in `[0, 1]`.

Placement and CTS distances and timing budgets must be finite and nonnegative.
Distances must fit in `2147483647` database units; timing budgets must remain
below STA's unconstrained range. Grid dimensions must be positive integers;
pair counts must be nonnegative integers. Zero requested pairs produces no
marks. Invalid options are rejected before changing the design or claims file,
including through the Python API.

Placement and CTS claims are staged in the output directory and replace the
requested file only after writing and closing succeed. The parent directory
must exist and be writable; the destination must be a regular file or a new
path. Devices, directories and output symlinks are rejected. If publication
fails, the command reports an error, preserves any previous claims file, and
restores its placement or clock-connection edits with refreshed parasitics.
Successful embedding with no selected pairs writes a header-only claims file,
replacing any previous claims. Such a file provides no ownership evidence.

### Set Routing Watermark

The `set_routing_watermark` command selects a keyed subset of signal nets, tags
each one, and returns the number tagged. A net is selected when the first four
bytes of `HMAC-SHA256(key, "net\0" || name)`, read as a little-endian integer
over 2^32, fall below the fraction. Previous tags are cleared first, so repeated
calls are idempotent. Call it before `detailed_route`.

```tcl
set_routing_watermark
    -key_hex key_hex
    [-fraction fraction]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-key_hex` | 64-character hex routing key. |
| `-fraction` | Finite fraction in `(0, 1]` of eligible signal nets to tag. Defaults to `0.05`. |

### Set Routing Watermark Strength

The `set_routing_watermark_strength` command sets the multiplier applied to the
non-preferred-direction grid cost when the detailed router routes a tagged net.
A value of `1` tags nets without biasing them, which is the control case.

This is router configuration and does not persist in the database, so it must be
set in the same process that runs `detailed_route`.

```tcl
set_routing_watermark_strength
    strength
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `strength` | Finite cost multiplier from 0 to 4294967040 (inclusive). |

The upper limit is the largest stored float below the router's unsigned cost
limit. Scaled edge costs and accumulated path/queue costs saturate at the
maximum router cost (4294967295) instead of wrapping. Very large strengths can make distinct expensive paths
indistinguishable; the default remains 100.

### Get Routing Watermark Strength

The `get_routing_watermark_strength` command returns the current multiplier.

```tcl
get_routing_watermark_strength
```

### Report Routing Watermark

The `report_routing_watermark` command ranks every routed signal net by its
wrong-way wirelength fraction and reports how many tagged nets fall below the
cutoff, with the resulting coincidence probability. Run it after
`detailed_route`.

```tcl
report_routing_watermark
    [-p p]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-p` | Finite quantile in `(0, 1)` below which a net counts as carrying the watermark. Defaults to `0.4`. |

### Clear Routing Watermark

The `clear_routing_watermark` command removes every watermark tag from the
current block and returns the number cleared.

```tcl
clear_routing_watermark
```

### Verify Watermark

The `verify_watermark` command checks the loaded design and returns `1` when it
carries the watermark. Ownership is granted when at least `-min_stages` of the
checked stages pass.

Placement and clock-tree marks are read from their claim files and judged by the
extraction rate, the fraction of claims that still hold, against `-tau`. An exact
match is not expected: routing and filling disturb a few marked objects.

CTS verification requires Liberty to identify buffers and sequential clock pins
using the same definitions as embedding. Missing Liberty is an error; a claimed
carrier with unknown Liberty fanout remains in the denominator and fails its
claim. Timing constraints are not required just to observe the fanout parity.

Routing has no claim file. Its marked set is recovered from `-routing_key_hex`
alone and judged by how improbable the marked nets' wrong-way wirelength is under
a random choice of marked set, against `-routing_alpha`. The sign of the
difference is not evidence on its own, since it is a coin flip on an unmarked
design.

```tcl
verify_watermark
    [-cts_claims file]
    [-min_stages n]
    [-placement_claims file]
    [-routing_alpha alpha]
    [-routing_fraction fraction]
    [-routing_key_hex key_hex]
    [-routing_permutations n]
    [-tau tau]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-cts_claims` | Claim file from the clock-tree watermark. |
| `-min_stages` | Stages that must pass. Defaults to `2`. |
| `-placement_claims` | Claim file from the placement watermark. |
| `-routing_alpha` | Largest p-value the routing stage may show and still pass. Defaults to `1e-4`. |
| `-routing_fraction` | The fraction the routing mark was embedded with. Defaults to `0.05`, matching `set_routing_watermark`. Supply the same fraction when embedding with a nondefault `-fraction`. |
| `-routing_key_hex` | 64-character hex routing key. Checks the routing stage. |
| `-routing_permutations` | Draws behind the raw sampled p-value, whose minimum is 1/(n+1) before the two-test adjustment. Defaults to `100000`. |
| `-tau` | Extraction rate a placement or clock-tree stage must reach. Defaults to `0.75`. |

At least one of `-placement_claims`, `-cts_claims` or `-routing_key_hex` is
required.

The routing stage also reports a closed-form bound on the same tail, exact when
the marked nets carry no wrong-way metal. That is the case a working watermark
produces, and it reaches probabilities sampling cannot express: on a routed jpeg
the sampled p-value floors at 1e-5 while the closed form gives 1e-515. The
decision uses `min(1, 2 * min(sampled_p, closed_form_bound))`. This Bonferroni
adjustment allocates half of `-routing_alpha` to each test and does not require
them to be independent. Taking the unadjusted minimum can exceed the configured
false-positive rate, because the sampled p-value is not an upper bound on the
exact tail. Both raw components remain in the report; C++ and Python callers can
use `RoutingStat::pValue()` to obtain the adjusted value used by Tcl.

The sampler compares marked subset sums directly. It counts ties, including
differences within a rounding allowance that scales with the number and magnitude
of the summed fractions. This can increase the reported p-value conservatively;
roundoff must not turn equal routing fractions into ownership evidence. Sorting
the fractions makes the result independent of net iteration order.

#### Claim file format

A claim file records what an embedder committed to. It is comma-separated with a
header row, and columns are matched by name, so a producer may emit them in any
order and add columns of its own. Missing required columns, empty or duplicate
header names, malformed row widths, and invalid fields in checkable claims are
errors; verification never scores a partially parsed file. The error identifies
the file and line. `skipped_reason` is required even when every value is empty.
Values are not quoted. Names must be nonempty, contain no commas, line breaks
or NULs, and have no leading or trailing spaces or tabs. Both embedders validate
all eligible instance names before selecting marks or changing the design and
fail explicitly if the format cannot represent a name.

Each row represents either a watermark claim or a candidate that was skipped.
A row is verified when `skipped_reason` is empty. The special value
`already_satisfied` is also treated as a valid claim: the object already matched
the target value and therefore required no modification.

Any other non-empty `skipped_reason` indicates that the candidate was not
included in the final claim set and is ignored during verification.
Once an object has been selected as a watermark claim, it remains a claim even
if embedding fails to achieve or preserve its target value. Such a failed claim
must still be recorded and will count against the extraction rate. This prevents
failed claims from being removed after observing the final design, which would
artificially inflate the extraction rate.


**Placement claims** describe pairs of cells in the same row. The bit is which of
the two sits further left, comparing instance bounding boxes.

| Column | Meaning |
| ----- | ----- |
| `kind` | `pair`; other kinds are ignored. |
| `A_name`, `B_name` | Instance names of the marked pair. |
| `target_bit` | `0` if A was driven left of B, `1` otherwise. |
| `skipped_reason` | Empty or `already_satisfied` to be checked. |

```text
kind,id,A_name,B_name,target_bit,skipped_reason
pair,_101770_|_101842_,_101842_,_101770_,0,already_satisfied
pair,_070839_|_070816_,_070839_,_070816_,1,
```

**CTS claims** describe leaf clock buffers. The bit is the parity of the buffer's
sequential fanout.

| Column | Meaning |
| ----- | ----- |
| `target_lcb` | Instance name of the marked leaf clock buffer. |
| `target_bit` | Parity the key called for, `0` or `1`. |
| `final_bit` | Parity the embedder achieved. Recorded for the owner; not read by the verifier. |
| `skipped_reason` | As above. |

```text
pair_idx,target_lcb,other_lcb,target_bit,final_bit,skipped_reason
0,clkbuf_leaf_314_clk,clkbuf_leaf_313_clk,1,1,
```

A claim naming an instance that is not in the design counts against the
extraction rate rather than aborting the check.

## Example scripts

Mark all three stages:

```tcl
detailed_placement
place_watermark -key_hex $place_key -claims_file wm_place.csv

clock_tree_synthesis -buf_list $buffers -root_buf $root_buf
set_propagated_clock [all_clocks]
estimate_parasitics -placement
cts_watermark -key_hex $cts_key -claims_file wm_cts.csv

set_routing_watermark -key_hex $route_key -fraction 0.02
set_routing_watermark_strength 100
global_route
detailed_route
```

Check a suspect layout, which need not be one this process built:

```tcl
read_db suspect.odb
verify_watermark -placement_claims wm_place.csv \
                 -cts_claims wm_cts.csv \
                 -routing_key_hex $route_key \
                 -routing_fraction 0.02
```

## Regression tests

There are a set of regression tests in `./test`. For more information, refer to
this [section](../../README.md#regression-tests).

Simply run the following script:

```shell
./test/regression
```

## Limitations

-   A technology whose router never wires against the preferred direction has no
    routing watermarking. The stage reports this and is skipped, and ownership rests
    on placement and the clock tree. ASAP7 is such a technology.
-   Watermark tags are not serialized to distributed workers, so the routing bias
    is not applied in distributed detailed routing.
-   Capacity is a property of the design. A sparse or timing-tight design may
    yield few placement pairs or none, and a shallow clock tree too few leaf
    buffers to pair.
-   Certificate sealing and the timestamped key commitment are not part of this
    module. Their guarantee comes from an independent timestamping authority
    rather than from anything the tool can check.

## Using the Python interface to wmk

The same work can be done from `openroad -python` through the `Watermark` object
on the design:

```python
from openroad import Tech, Design
import wmk

watermark = design.getWatermark()
options = wmk.PlacementOptions()
watermark.placementWatermark(key_hex, options, "wm_place.csv")
watermark.selectNetsKeyed(key_hex, 0.02)
result = watermark.verifyPlacement("wm_place.csv")
```

A key is passed as the 64-character hex string or as 32 raw bytes. Anything else
is refused. Key generation has no Python entry point; use
`design.evalTclString("generate_watermark_key -design_id ...")`.

## References

1.  A. B. Kahng and Y. Liu. Kerckhoffs-Compliant Watermarking for Physical Design
    IP Protection: From Placement to Routing. arXiv preprint arXiv:2608.05055.
    [(arXiv)](https://arxiv.org/pdf/2608.05055)


## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
