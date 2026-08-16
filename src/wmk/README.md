# Watermark

The Watermark module embeds and checks keyed ownership evidence in a physical
design, implementing the routing watermark of Kahng et al., "Robust IP
Watermarking Methodologies for Physical Design" (ISPD'98) and the verification
side of the PDMarks scheme.

A keyed subset of signal nets is selected with HMAC-SHA256 and tagged with a
`watermark` property. During detailed routing those nets pay an inflated cost
for wiring against a layer's preferred direction, so they end up using
measurably less wrong-way wire than the rest of the design. Because the
selection is keyed, the tagged set cannot be predicted without the key, and the
algorithm can be public.

Two further stages are marked the same way. After detailed placement, a keyed
subset of same-row same-width cell pairs is put into a keyed left-to-right
order; after clock tree synthesis, a keyed subset of leaf clock buffers is
driven to a keyed sequential-fanout parity. All three stages are embedded and
verified by this module, with no other tooling.

Which objects each stage marks is decided before the key is consulted. An
embedder that chose whichever pair already happened to match would report a
perfect extraction rate on a design it had never touched, and the rate would be
no evidence of anything; see [Verify Watermark](#verify-watermark).

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

### Place Watermark

The `place_watermark` command puts a keyed subset of cell pairs into a keyed
left-to-right order, writing the pairs it claimed to `-claims_file` and
returning how many there were. Run it after detailed placement.

Only cells that sit in the same row and have the same width are paired, so a
swap leaves the row legal and the area unchanged. Candidates are screened on
slack and on the wirelength the swap would cost, and the design is re-legalized
afterwards.

Every pair chosen is claimed, including any whose swap legalization then undid.
The message reports how many no longer hold, which is the number to watch: a
mark that did not survive its own embedding will not survive routing either.

```tcl
place_watermark
    -key_hex key_hex
    -claims_file file
    [-grid_nx n]
    [-grid_ny n]
    [-hpwl_eps_dbu eps]
    [-max_disp_um disp]
    [-pair_dist_um dist]
    [-pairs_per_tile n]
    [-slack_threshold_ns slack]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-key_hex` | A 64-character hex string, the 32-byte placement key. It fixes each pair's target order and nothing else; which cells are paired does not depend on it. |
| `-claims_file` | Where to write the claims, in the format below. |
| `-grid_nx`, `-grid_ny` | Tiles across and down the core. Marks are spread over the tiling rather than clustering wherever candidates are densest. Both default to 8. |
| `-hpwl_eps_dbu` | Largest half-perimeter wirelength change, in database units, a swap may cost. Defaults to 100. |
| `-max_disp_um` | How far re-legalization may move a cell, in microns. Defaults to 5. |
| `-pair_dist_um` | How far along the row to look for a partner, in microns. Defaults to 1.0. |
| `-pairs_per_tile` | Most pairs to claim per tile. Defaults to 4. |
| `-slack_threshold_ns` | Cells with less slack than this are left alone, so the mark stays off the paths that set the clock period. Defaults to 0.20. Set to 0 to disable the screen. |

Capacity depends on how densely the design is packed. A design with few
same-row same-width neighbours, or one whose swaps all cost wirelength, may
yield no pairs at all; that is a property of the design, not a failure.

### CTS Watermark

The `cts_watermark` command drives a keyed subset of leaf clock buffers to a
keyed sequential-fanout parity, writing the pairs it claimed to `-claims_file`
and returning how many there were. Run it after clock tree synthesis.

Buffers are marked in pairs, and the parity is changed by moving one
flip-flop's clock pin from one buffer of the pair to the other. That leaves the
flop clocked and the tree connected, and it survives anything that does not add
or remove a sink -- routing, filling and metal fixes all preserve it.

What it can cost is skew, so a move is undone if it makes the clock's worst skew
worse. The pair is still claimed; the message reports how many pairs ended at
the parity the key asked for.

```tcl
cts_watermark
    -key_hex key_hex
    -claims_file file
    [-num_pairs n]
    [-sibling_dist_um dist]
    [-skew_margin_ns margin]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-key_hex` | A 64-character hex string, the 32-byte clock-tree key. It fixes which buffer of each pair carries the mark and what parity it must show. |
| `-claims_file` | Where to write the claims, in the format below. |
| `-num_pairs` | Most pairs to mark. Each carries one bit. Defaults to 32. |
| `-sibling_dist_um` | Largest distance between the two buffers of a pair, in microns, so a moved sink stays local. Defaults to 20.0. |
| `-skew_margin_ns` | How much worse the clock's worst skew may get, in nanoseconds. Defaults to 0, meaning no worse than before. |

### Set Routing Watermark

The `set_routing_watermark` command selects a keyed subset of signal nets and
tags each one, returning the number tagged. Any previous tags are cleared
first, so repeated calls are idempotent. Call it before `detailed_route`.

```tcl
set_routing_watermark
    -key_hex key_hex
    [-fraction fraction]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-key_hex` | A 64-character hex string, the 32-byte routing key. Each net is selected when the first four bytes of `HMAC-SHA256(key, "net\0" + name)`, read as a little-endian integer over 2^32, fall below `-fraction`. |
| `-fraction` | Expected fraction of eligible signal nets to tag. Each net is an independent draw, so the realized count varies. Defaults to 0.05. |

### Set Routing Watermark Strength

The `set_routing_watermark_strength` command sets the multiplier applied to the
non-preferred-direction grid cost when the detailed router routes a tagged net.
A value of 1 tags nets without biasing them, which is the control case for
measuring the watermark's effect.

This is router configuration, not design data: it does not persist in the
database, so it must be set in the same process that runs `detailed_route`.

```tcl
set_routing_watermark_strength
    strength
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `strength` | Non-negative cost multiplier. |

### Get Routing Watermark Strength

The `get_routing_watermark_strength` command returns the current multiplier.

```tcl
get_routing_watermark_strength
```

### Report Routing Watermark

The `report_routing_watermark` command ranks every routed signal net by its
wrong-way wirelength fraction and reports how many tagged nets fall below the
cutoff, together with the coincidence probability. Run it after
`detailed_route`.

```tcl
report_routing_watermark
    [-p p]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-p` | Quantile cutoff below which a net counts as carrying the watermark. Defaults to 0.4. |

### Clear Routing Watermark

The `clear_routing_watermark` command removes every `watermark` tag from the
current block and returns the number cleared.

```tcl
clear_routing_watermark
```

### Verify Watermark

The `verify_watermark` command checks the loaded design against whichever
stages it is given, and returns 1 when every stage it checked passed.

Placement and clock-tree marks are checked from their claim files. No key is
needed: it was consumed at embed time to derive the target values, which the
claim files record. Verification re-observes each claimed object and compares
it to the claimed value. Ownership is decided by the extraction rate, the
fraction of claims that still hold, rather than by an exact match, because
routing and filling legitimately disturb a few marked objects.

The routing mark has no claim file. It is a population effect rather than a set
of per-object values, and the marked set is recovered from the key alone, so
that stage needs `-routing_key_hex` and nothing from embed time. The statistic
is the difference in mean wrong-way wirelength fraction between the marked nets
and the rest. Its sign is not evidence -- on a design carrying no watermark it
is a coin flip -- so the stage is decided on how improbable the value is under
a random choice of marked set, against `-routing_alpha`.

```tcl
verify_watermark
    [-cts_claims file]
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
| `-cts_claims` | Claim file from the clock-tree watermark. Each claim names a leaf clock buffer and the parity its sequential fanout was driven to. |
| `-placement_claims` | Claim file from the placement watermark. Each claim names a pair of cells and which of the two was driven to sit further left. |
| `-routing_alpha` | Largest p-value the routing stage may show and still pass. Defaults to 1e-4. |
| `-routing_fraction` | The fraction the routing mark was embedded with. It must match, or the recovered set will not be the marked one. Defaults to 0.02. |
| `-routing_key_hex` | A 64-character hex string, the 32-byte routing key. Checks the routing stage. |
| `-routing_permutations` | Draws behind the routing p-value, which is floored at 1/(n+1). Defaults to 100000. |
| `-tau` | Extraction rate a placement or clock-tree stage must reach to pass. Defaults to 0.75. |

At least one of `-placement_claims`, `-cts_claims` or `-routing_key_hex` is
required.

The routing stage also reports a closed-form bound on the same tail, which is
exact when the marked nets carry no wrong-way metal at all. That is the case a
working watermark produces, and it reaches probabilities far below anything
sampling can express: on a routed jpeg the sampled p-value bottoms out at its
floor of 1e-5 while the closed form gives 1e-515.9. The decision uses whichever
is smaller.

#### Claim file format

A claim file records what an embedder committed to: which objects it marked and
what value it drove each one to. It is the evidence a verifier checks, so its
format is defined here rather than by whichever tool produced it.

The file is comma-separated with a header row naming the columns. Columns are
matched by name, so a producer may emit them in any order and may add columns of
its own; the ones below are the only ones read. Values are not quoted, and
instance names must therefore not contain commas.

A row is checked unless `skipped_reason` is non-empty, with one exception:
`already_satisfied` means the object already carried the target value and needed
no edit, which is still a claim the owner can verify.

`skipped_reason` marks a candidate the embedder never claimed, and nothing else.
A pair it selected, tried to mark and failed to mark is a claim like any other,
and must be written as one: it will not hold, and the extraction rate will say
so. Excusing it instead would let a claim file choose its own denominator, and
a rate computed that way is 1 on every design, marked or not, which is no
evidence at all. For the same reason the verifier does not consult a row's
record of how its own embedding turned out.

**Placement claims** describe pairs of cells in the same row. The bit is which of
the two sits further left, comparing the x coordinate of each instance bounding
box.

| Column | Meaning |
| ----- | ----- |
| `kind` | `pair`; rows of any other kind are ignored. |
| `A_name`, `B_name` | Instance names of the marked pair. |
| `target_bit` | `0` if A was driven to sit left of B, `1` otherwise. |
| `skipped_reason` | Empty or `already_satisfied` to be checked; any other value skips the row. |

```text
kind,id,A_name,B_name,target_bit,skipped_reason
pair,_101770_|_101842_,_101842_,_101770_,0,already_satisfied
pair,_070839_|_070816_,_070839_,_070816_,1,
```

**CTS claims** describe leaf clock buffers. The bit is the parity of the buffer's
sequential fanout: the number of sequential clock sinks on the net its single
output drives.

| Column | Meaning |
| ----- | ----- |
| `target_lcb` | Instance name of the marked leaf clock buffer. |
| `target_bit` | Parity the key called for, `0` or `1`. This is what is checked. |
| `final_bit` | Parity the embedder managed to leave behind. Recorded for the owner's benefit and not read by the verifier, for the reason given above. |
| `skipped_reason` | As above. |

```text
pair_idx,target_lcb,other_lcb,target_bit,final_bit,skipped_reason
0,clkbuf_leaf_314_clk,clkbuf_leaf_313_clk,1,1,
```

A claim naming an instance that is not in the design counts against the
extraction rate rather than aborting the check, so a partially disturbed layout
still produces a verdict.

## Example scripts

Tag a keyed subset, route with the bias applied, then report:

```tcl
set_routing_watermark -key_hex $key -fraction 0.02
set_routing_watermark_strength 100
detailed_route
report_routing_watermark
```

Check a suspect layout:

```tcl
read_db suspect.odb
verify_watermark -placement_claims wm_place_embed.csv \
                 -cts_claims wm_cts_embed.csv
```

## Regression tests

```shell
./test/regression
```

## Limitations

A layer that is routed strictly in one direction produces no wrong-way
wirelength, so the routing statistic is undefined on such a technology and the
watermark carries no signal there.

The tag is mirrored onto the router's own net objects when the design is read,
and those objects are not serialized to distributed workers, so the bias is not
applied in distributed detailed routing.

How many bits a design can carry is a property of the design. A sparse or
timing-tight design may yield few placement pairs or none, and a shallow clock
tree may have too few leaf buffers to pair. The commands report what they
committed; a design that commits nothing has no placement or clock-tree
evidence to offer, and the routing stage has to stand on its own.

## FAQ

Please refer to the [GitHub issues](https://github.com/The-OpenROAD-Project/OpenROAD/issues).

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
