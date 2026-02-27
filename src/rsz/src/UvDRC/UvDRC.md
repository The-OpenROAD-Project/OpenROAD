# UvDRC (Univista DRC fixing project)

## Regrs

- 11: my area 7, openroad 4. (different delay model?)
- 12: violation after `repair_design`
  - [x] max cap 使用了整形，导致为0。
  - [x] Buffer Cell 的 load cap 限制为 INF。要看 OpenROAD 是怎么得到这一数据的。
    - `MaxLengthForCap()` 中用到的 `out->capacitanceLimit()`
- 17: crash
  - 当 alpha 为 0 时，OpenROAD 不会使用 PD 算法，而是调用 flute 引擎做 virtual routing，其返回的 SteinerTree 数据结构会有一些不合标准之处。

## TODOs

1. Support slew & cap margin settings
2. Benchmark tests
