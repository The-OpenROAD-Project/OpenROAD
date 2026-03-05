# UvDRC (Univista DRC fixing project)

## TODOs

1. Support slew & cap margin settings
2. Benchmark tests

## LIMITATION

一些 case 里，OpenROAD 会认为 PD 算法得到的 SteinerTree 有 overlap，判为不合法。此时它会调用 Flute 引擎做一个快速的 virtual routing 得到一颗 RC 树。但我们现有的代码是基于 PD 算法的 SteinerTree 写的，用这颗 RC 树可能 CRASH。

## WEEKLY

1. 修 BUG，目前已经能够修复 cap violation
2. TODO: 代码进到我们的 branch 里
3. 更多的 BM 测试
4. 研究 OpenROAD 在 Routing 后 fix slew 的代码
