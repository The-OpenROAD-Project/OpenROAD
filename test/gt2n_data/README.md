# GT2N PDK

GT2N is an open 2nm nanosheet PDK with backside power delivery (BSPDN).
Standard cell `vdd` / `vss` pins sit on `BPR` (buried power rail) and the
power grid is built entirely on backside metal, which makes it the only
PDK in this tree that exercises backside power on realistic data.

Shared like `Nangate45` / `asap7` / `sky130hd`: symlinked into module test
directories rather than copied.

## Contents

| File | Origin |
|---|---|
| `gt2_tech.lef` | GT2N `techlib/gt2_tech.lef`, verbatim |
| `gt2_6t_w31_svt.lef` | GT2N `lef/tt/gt2_6t_w31_svt.lef`, verbatim |
| `gt2_6t_w31_svt_tt_0p7v25c.lib.gz` | GT2N `lib/tt/...`, gzipped |
| `setRC.tcl` | transcribed from the lambdapdk gt2n PDK RC table |

Only the `w31_svt` library is carried. The PDK ships five VT flavors
(svt / hvt / lvt / ulvt / elvt); pulling in all of them would mean five
standard-cell LEFs and five liberties for no added coverage, so designs
built against this directory should restrict synthesis to a single library.

The layer stack is `BPR` / `BV0` / `BM1` / `BV1` / `BM2` / ... / `BRDL` on
the backside, and `M0..M4` and up on the frontside.

The tech LEF carries no `RESISTANCE` / `CAPACITANCE` properties, so anything
doing parasitic or IR analysis must `source setRC.tcl` first -- otherwise
PSM fails with `PSM-0021`, "Resistance map contains invalid values".

## Consumers

* `src/psm/test/gcd_gt2n_backside` -- IR drop analysis on a backside power
  grid. Design data in `src/psm/test/gt2n_gcd_data`.

## License

BSD-3-Clause -- the same license as OpenROAD itself -- Copyright 2025
Dongwon Jang, Piyush Kumar, Da Eun Shim, Akshata Ashoka, Meghana
Mallikarjuna, Azad Naeemi, or Georgia Institute of Technology (the exact
author list varies per file). The files are shipped verbatim, so each one
retains its own full BSD-3-Clause header, which is what clause 1 requires;
this mirrors how `../asap7/` carries its Arizona State University headers.
Obtained via <https://github.com/siliconcompiler/lambdapdk> (PDK sources
fetched by `lambdapdk.gt2n`).

The PDK asks that published work using it cite:

> D. Jang, P. Kumar, M. N. H. Shazon, S. J. Ram, A. Svizhenko, V. Moroz,
> A. Ceyhan, N. A. Radhakrishn, and A. Naeemi, "GT2N: An Open-Source 2nm
> Nanosheet PDK Enabling Multi-Width/VT Benchmarking," IEEE International
> Symposium on Circuits and Systems (ISCAS) 2026.
