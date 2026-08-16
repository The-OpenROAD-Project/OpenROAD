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

Placement and clock-tree watermarks are embedded by the flow scripts; this
module verifies them.

## Commands

```{note}
- Parameters in square brackets `[-param param]` are optional.
- Parameters without square brackets `-param2 param2` are required.
```

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

The `verify_watermark` command checks committed placement and clock-tree claims
against the loaded design and returns 1 when every stage that had claims met
the threshold.

The key is not required. It was consumed when the watermark was embedded, to
derive the target values that the claim files record. Verification re-observes
each claimed object and compares it to the committed value.

Ownership is decided by the extraction rate, the fraction of claims that still
hold, rather than by an exact match: routing and filling legitimately disturb a
few marked objects.

```tcl
verify_watermark
    [-cts_claims file]
    [-placement_claims file]
    [-tau tau]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-cts_claims` | Claim file from the clock-tree watermark. Each claim names a leaf clock buffer and the parity its sequential fanout was driven to. |
| `-placement_claims` | Claim file from the placement watermark. Each claim names a pair of cells and which of the two was driven to sit further left. |
| `-tau` | Extraction rate a stage must reach to pass. Defaults to 0.75. |

At least one claim file is required.

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
| `target_bit` | Parity the sequential fanout was driven to, `0` or `1`. |
| `final_bit` | Parity actually achieved. A row whose `final_bit` differs from `target_bit` was not committed and is skipped. |
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

## FAQ

Please refer to the [GitHub issues](https://github.com/The-OpenROAD-Project/OpenROAD/issues).

## License

BSD 3-Clause License. See [LICENSE](../../LICENSE) file.
