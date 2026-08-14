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
    [-key_hex key_hex]
    [-message message]
    [-fraction fraction]
```

#### Options

| Switch Name | Description |
| ----- | ----- |
| `-key_hex` | A 64-character hex string, the 32-byte routing key. Each net is selected when the first four bytes of `HMAC-SHA256(key, "net\0" + name)`, read as a little-endian integer over 2^32, fall below `-fraction`. |
| `-message` | Legacy unkeyed selection, seeded by a public string. Superseded by `-key_hex`; the two are mutually exclusive. |
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
