# `vlsiffra` ASAP7 ORFS smoke tests

This package adds two full-flow ASAP7 smoke tests for [OpenROAD issue
#2361](https://github.com/The-OpenROAD-Project/OpenROAD/issues/2361) using
upstream examples from
[`antonblanchard/vlsiffra`](https://github.com/antonblanchard/vlsiffra) at
commit `22e7acc55020f5f09092dd37b765191549b57ada`.

The checked-in RTL is intentionally vendored so the tests remain hermetic and
do not depend on Amaranth or generator tooling at test time.

The upstream `vlsiffra` project is licensed under Apache-2.0. The vendored
generated RTL in this directory is kept with explicit source provenance for
that reason.

## Sources

- 32-bit 4-cycle ASAP7 multiplier:
  [README](https://github.com/antonblanchard/vlsiffra/blob/22e7acc55020f5f09092dd37b765191549b57ada/openroad/32bit_4cycle_asap7_multiplier/README.md)
- 64-bit 2-cycle ASAP7 multiply-adder:
  [README](https://github.com/antonblanchard/vlsiffra/blob/22e7acc55020f5f09092dd37b765191549b57ada/openroad/64bit_2cycle_asap7_multiply_adder/README.md)

## RTL provenance

`multiplier.v` was generated with:

```sh
vlsi-multiplier --bits=32 --algorithm=koggestone --tech=asap7 \
  --register-post-ppa --register-post-ppg --register-output \
  --output=multiplier.v
```

`multiply_adder.v` was generated with:

```sh
vlsi-multiplier --bits=64 --multiply-add --algorithm=hancarlson \
  --tech=asap7 --register-post-ppg --output=multiply_adder.v
```
