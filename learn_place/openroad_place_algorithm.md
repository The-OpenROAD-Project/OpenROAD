# OpenROAD 当前 Placement 算法架构与核心流程

本文参考 [`placement_overview.md`](./placement_overview.md) 的组织方式，结合 OpenROAD 当前源码，梳理 OpenROAD 中 place 相关模块的代码架构，并重点分析其核心算法。

OpenROAD 的 placement 并不是一个单一算法，而是一条由多个 placer 协同组成的工程流水线。典型流程可以概括为：

```text
IO / Pad Placement
  → Macro Placement
  → Global Placement
  → Legalization / Detailed Placement
  → Filler Placement / Placement Check
```

其中，标准单元布局最核心的是：

```text
gpl: RePlAce 风格的静电场全局布局
  → dpl: OpenDP 合法化与详细布局
```

---

## 一、代码架构总览

OpenROAD 中 placement 相关模块主要分布在 `src` 下的几个目录中。

| 模块 | 目录 | 主要职责 |
|---|---|---|
| `gpl` | [`../src/gpl`](../src/gpl) | Global Placement，全局布局，基于 RePlAce / ePlace 风格的解析式非线性布局算法 |
| `dpl` | [`../src/dpl`](../src/dpl) | Detailed Placement，包括 legalization、cell snapping、局部优化、filler placement、placement check |
| `mpl` | [`../src/mpl`](../src/mpl) | Macro Placement，基于 RTLMP / Hier-RTLMP 的层次化宏单元布局 |
| `ppl` | [`../src/ppl`](../src/ppl) | Pin Placement / IO placement，处理 IO pin 位置 |
| `pad` | [`../src/pad`](../src/pad) | Pad / bump / corner cell 等 IO ring 相关放置 |

如果只关注标准单元 placement，最核心的模块是：

```text
src/gpl  →  src/dpl
```

如果设计中包含大量 macro，则前面还会经过：

```text
src/mpl  →  src/gpl  →  src/dpl
```

---

## 二、整体 Place 流程

OpenROAD 的 placement 流程可以分为四类对象：

1. **IO pin / pad**
2. **macro / block**
3. **standard cell**
4. **filler cell**

典型关系如下：

```text
Floorplan 已建立 core / rows / tracks
        │
        ▼
IO / Pad Placement
        │
        ▼
Macro Placement, if macros exist
        │
        ▼
Global Placement: gpl
        │
        ▼
Detailed Placement / Legalization: dpl
        │
        ▼
Filler Placement
        │
        ▼
Check Placement
```

在算法层面，OpenROAD 当前的标准单元 placement 主线是：

```text
初始布局 BiCGSTAB
        │
        ▼
Nesterov 静电场全局布局
        │
        ▼
Diamond Search 或 Negotiation Legalizer 合法化
        │
        ▼
局部 detailed placement / filler / legality check
```

---

## 三、Global Placement：`gpl` 模块

### 1. 模块定位

`gpl` 是 OpenROAD 的全局布局器，入口文档是 [`../src/gpl/README.md`](../src/gpl/README.md)。该模块基于开源 RePlAce，核心思想来自 ePlace / RePlAce 系列论文。

OpenROAD 文档中明确说明，`gpl` 使用：

- analytic placement
- nonlinear placement
- electrostatic force model
- Nesterov accelerated gradient descent
- routability-driven placement
- timing-driven placement
- mixed-size placement

对应命令是：

```tcl
global_placement
```

核心源码入口：

| 文件 | 作用 |
|---|---|
| [`../src/gpl/src/replace.cpp`](../src/gpl/src/replace.cpp) | `Replace` 类，global placement 的主控入口 |
| [`../src/gpl/src/replace.tcl`](../src/gpl/src/replace.tcl) | Tcl 命令 `global_placement` 的参数解析入口 |
| [`../src/gpl/src/placerBase.cpp`](../src/gpl/src/placerBase.cpp) | 从 OpenDB 构造 placement 内部数据结构 |
| [`../src/gpl/src/initialPlace.h`](../src/gpl/src/initialPlace.h) | 初始布局阶段 |
| [`../src/gpl/src/solver.cpp`](../src/gpl/src/solver.cpp) | BiCGSTAB 稀疏线性方程求解 |
| [`../src/gpl/src/nesterovPlace.cpp`](../src/gpl/src/nesterovPlace.cpp) | Nesterov 全局布局主循环 |
| [`../src/gpl/src/nesterovBase.cpp`](../src/gpl/src/nesterovBase.cpp) | wirelength、density、bin、gradient 等核心计算 |
| [`../src/gpl/src/routeBase.cpp`](../src/gpl/src/routeBase.cpp) | routability-driven placement，RUDY / congestion 相关 |
| [`../src/gpl/src/timingBase.cpp`](../src/gpl/src/timingBase.cpp) | timing-driven placement，critical net reweight 相关 |

---

### 2. `gpl` 的主控入口

`Replace::doPlace()` 是标准全局布局主流程：

```text
doPlace()
  → doInitialPlace()
  → doNesterovPlace()
```

源码位置：[`../src/gpl/src/replace.cpp`](../src/gpl/src/replace.cpp)

也就是说，OpenROAD 的 global placement 不是直接进入 Nesterov，而是先做一个初始布局，再进入非线性优化。

---

### 3. 初始布局：BiCGSTAB 求解线性系统

初始布局阶段由 `doInitialPlace()` 创建 `InitialPlace`，然后调用：

```text
InitialPlace::doBicgstabPlace()
```

其底层稀疏线性方程求解在 [`../src/gpl/src/solver.cpp`](../src/gpl/src/solver.cpp)。核心是 Eigen 的：

```text
BiCGSTAB<SMatrix, IdentityPreconditioner>
```

这一阶段可以理解为：

```text
把 net 近似成弹簧系统
  → 构造稀疏线性系统
  → 分别求解 x / y 坐标
  → 得到一个较合理的初始布局
```

其作用是给后续 Nesterov 非线性优化一个更好的起点，避免从随机或中心点开始造成较差收敛。

---

### 4. 核心算法：静电场模型 + Nesterov 加速梯度下降

`gpl` 的核心算法在 [`../src/gpl/src/nesterovPlace.cpp`](../src/gpl/src/nesterovPlace.cpp) 和 [`../src/gpl/src/nesterovBase.cpp`](../src/gpl/src/nesterovBase.cpp)。

数学上，global placement 被建模为一个连续可微的非线性优化问题：

$$
\min_x \quad W(x) + \lambda D(x)
$$

其中：

- $x$：所有 movable instances 的连续坐标。
- $W(x)$：平滑化的线长目标。
- $D(x)$：密度惩罚目标，用于消除 overlap。
- $\lambda$：密度惩罚系数。

可以把它理解为：

```text
线长目标 W(x)：像弹簧，拉近相连 cell
密度目标 D(x)：像电荷排斥，把重叠 cell 推开
```

#### 4.1 Wirelength：平滑 HPWL

真实 HPWL 包含 `max` / `min`，不可导。解析式布局需要可微目标，因此 RePlAce 风格算法通常使用平滑线长近似。

OpenROAD 的 `gpl` 在 Nesterov 迭代中会更新 wirelength force：

```text
NesterovBaseCommon::updateWireLengthForceWA()
```

其中 `WA` 可以理解为 weighted-average wirelength 形式。

#### 4.2 Density：bin 网格 + 静电势场

密度目标的核心思想是把 layout 区域切成 bin grid：

```text
core area
  → bin grid
  → 统计每个 bin 的 cell density
  → 计算 overflow
  → 通过静电势场产生密度梯度
```

相关计算集中在 [`../src/gpl/src/nesterovBase.cpp`](../src/gpl/src/nesterovBase.cpp)，其中会维护：

- `GCell`
- `Bin`
- density location
- density size
- overflow
- density gradient

这对应 `placement_overview.md` 中提到的静电场模型：

```text
cell 视作电荷
密度过高处产生排斥力
通过梯度把 cell 推向低密度区域
```

#### 4.3 Nesterov 主循环

Nesterov 主循环在 `NesterovPlace::doNesterovPlace()` 中。

核心迭代结构可以概括为：

```text
for iter in max_iter:
    计算 Nesterov 加速系数
    backtracking line search
    更新 cell 坐标
    更新 wirelength / density gradient
    更新 step length
    更新 overflow / HPWL
    根据 overflow 调整 wirelength coefficient
    如果 timing-driven，重加权 critical nets
    如果 routability-driven，做 congestion 估计和 cell inflation
    判断收敛 / 发散 / revert
```

源码中的关键步骤包括：

- `doBackTracking()`：回溯线搜索，寻找合适步长。
- `nesterovUpdateCoordinates()`：根据 Nesterov 方向更新坐标。
- `npUpdateNextGradient()`：更新下一轮梯度。
- `nesterovUpdateStepLength()`：更新 step length。
- `updateNextIter()`：更新 overflow、HPWL、wirelength coefficient 等迭代状态。
- `isConverged()`：判断是否达到目标 overflow。

可以把 `gpl` 的主优化过程理解为：

```text
既要缩短线长
又要把 cell 从拥挤区域推开
并且在每一轮根据 overflow 动态调节线长项和密度项的相对强度
```

---

### 5. Timing-driven Global Placement

`global_placement -timing_driven` 会启用 timing-driven placement。

OpenROAD 文档说明，`gpl` 会执行类似 virtual `repair_design` 的流程来获得 slack 信息，然后对低 slack 的 net 增加权重。

直观理解：

```text
关键路径 slack 越差
  → 对应 net 权重越大
  → 线长项中的弹簧更强
  → 关键 cell 被拉得更近
  → 有助于降低 wire delay
```

相关源码：[`../src/gpl/src/timingBase.cpp`](../src/gpl/src/timingBase.cpp)

触发机制不是每一轮都做 STA，而是根据 overflow 阈值触发 timing net reweight。

---

### 6. Routability-driven Global Placement

`global_placement -routability_driven` 会启用 routability-driven placement。

核心思想是：

```text
先估计 routing congestion
  → 找到 congestion hotspot
  → 对热点区域 cell 做 inflation
  → 等价于增加这些 cell 的密度面积
  → Nesterov 后续迭代会把它们推开
```

OpenROAD 支持两种拥塞来源：

- 默认使用 RUDY，速度快。
- 可选使用 `-routability_use_grt` 调用 global router，精度更高但更慢。

相关源码：[`../src/gpl/src/routeBase.cpp`](../src/gpl/src/routeBase.cpp)

从优化角度看，这相当于把原本只考虑 placement density 的问题扩展为：

```text
cell density + routing demand density
```

即：布局不仅要不重叠，还要给布线留空间。

---

## 四、Macro Placement：`mpl` 模块

### 1. 模块定位

`mpl` 是 macro placer，文档入口是 [`../src/mpl/README.md`](../src/mpl/README.md)。当前 OpenROAD 使用的是 RTLMP / Hier-RTLMP 风格的层次化宏单元布局方法。

主要命令：

```tcl
rtl_macro_placer
place_macro
set_macro_guidance_region
set_macro_base_halo
set_macro_halo
block_macro_channels
```

核心源码：

| 文件 | 作用 |
|---|---|
| [`../src/mpl/src/hier_rtlmp.cpp`](../src/mpl/src/hier_rtlmp.cpp) | Hier-RTLMP 主体流程 |
| [`../src/mpl/src/rtl_mp.cpp`](../src/mpl/src/rtl_mp.cpp) | RTL macro placement 相关实现 |
| [`../src/mpl/src/clusterEngine.cpp`](../src/mpl/src/clusterEngine.cpp) | 层次化 clustering |
| [`../src/mpl/src/SimulatedAnnealingCore.cpp`](../src/mpl/src/SimulatedAnnealingCore.cpp) | 模拟退火核心框架 |
| [`../src/mpl/src/SACoreHardMacro.cpp`](../src/mpl/src/SACoreHardMacro.cpp) | hard macro 的 SA cost / move |
| [`../src/mpl/src/SACoreSoftMacro.cpp`](../src/mpl/src/SACoreSoftMacro.cpp) | soft macro 的 SA cost / move |
| [`../src/mpl/src/pusher.cpp`](../src/mpl/src/pusher.cpp) | macro boundary push / overlap repair |
| [`../src/mpl/src/snapper.cpp`](../src/mpl/src/snapper.cpp) | macro snapping，对齐 grid / track |

---

### 2. Macro Placement 的算法思想

与标准单元不同，macro 数量较少、面积巨大、形状强约束明显，因此不适合直接用标准单元的连续密度模型处理。

`mpl` 的基本思路是：

```text
根据 RTL 层次和 netlist 连接关系做 clustering
  → 构造层次化物理规划树
  → 对 macro / cluster 做 floorplan 搜索
  → 使用 simulated annealing 优化 cost
  → snap / push / repair 得到合法 macro placement
```

其 cost function 会综合多个目标：

- area cost
- outline violation
- wirelength
- guidance region
- fence violation
- boundary cost
- notch cost
- soft blockage cost

这说明 macro placement 更像是一个：

```text
层次化 floorplanning + 组合优化 / 模拟退火问题
```

而不是 `gpl` 那种大规模连续可微优化问题。

---

## 五、Detailed Placement / Legalization：`dpl` 模块

### 1. 模块定位

`dpl` 是 OpenROAD 的 detailed placement 模块，基于 OpenDP。入口文档是 [`../src/dpl/README.md`](../src/dpl/README.md)。

主要命令包括：

```tcl
detailed_placement
set_placement_padding
filler_placement
remove_fillers
check_placement
optimize_mirroring
improve_placement
```

核心源码：

| 文件 | 作用 |
|---|---|
| [`../src/dpl/src/Opendp.cpp`](../src/dpl/src/Opendp.cpp) | `Opendp` 主控入口 |
| [`../src/dpl/src/Place.cpp`](../src/dpl/src/Place.cpp) | 默认 diamond search legalization |
| [`../src/dpl/src/Optdp.cpp`](../src/dpl/src/Optdp.cpp) | detailed placement improvement |
| [`../src/dpl/src/NegotiationLegalizer.cpp`](../src/dpl/src/NegotiationLegalizer.cpp) | negotiation legalizer 主流程 |
| [`../src/dpl/src/NegotiationLegalizerPass.cpp`](../src/dpl/src/NegotiationLegalizerPass.cpp) | rip-up and replace、history cost、greedy improve 等 |
| [`../src/dpl/src/PlacementDRC.cpp`](../src/dpl/src/PlacementDRC.cpp) | placement legality / DRC 检查 |

---

### 2. `dpl` 主流程

`detailed_placement` 的 C++ 入口是：

```text
Opendp::detailedPlacement()
```

源码位置：[`../src/dpl/src/Opendp.cpp`](../src/dpl/src/Opendp.cpp)

主要步骤：

```text
importDb()
  → adjustNodesOrient()
  → 统计 utilization / HPWL
  → 根据模式选择 legalizer
      ├─ diamondDPL()
      └─ NegotiationLegalizer
  → findDisplacementStats()
  → updateDbInstLocations()
```

`dpl` 的目标不是重新做全局优化，而是把 `gpl` 产生的连续坐标转换成满足物理约束的合法位置：

- 对齐 standard cell rows
- 对齐 site grid
- 消除 overlap
- 满足 fixed cell / blockage / fence region
- 尽量降低 displacement
- 尽量少破坏 HPWL

---

### 3. 默认合法化：Diamond Search

默认 engine 是 diamond search，入口在：

```text
Opendp::diamondDPL()
```

源码位置：[`../src/dpl/src/Place.cpp`](../src/dpl/src/Place.cpp)

其核心函数是：

```text
Opendp::diamondSearch()
```

算法思想：

```text
对每个 cell：
    从 global placement 给出的目标位置开始
    按 Manhattan distance 向外扩展搜索
    搜索形状像菱形 diamond
    找到第一个合法 site 后放置
```

源码中使用 priority queue 按 Manhattan distance 选择候选点：

```text
center
  → distance 0
  → distance 1
  → distance 2
  → ...
```

每个候选位置会调用：

```text
canBePlaced()
  → checkPixels()
  → checkRegionOverlap()
```

用于检查：

- 是否越界
- 是否与已放置 cell / fixed object 冲突
- 是否满足 fence region
- 是否满足 row / site grid

这个算法非常工程化：简单、稳定、速度快，适合作为默认 legalization engine。

---

### 4. 可选合法化：Negotiation Legalizer

`detailed_placement -use_negotiation` 会启用 negotiation legalizer。

文档中说明它基于 NBLG 思路，整体是两阶段或多阶段 legalizer：

```text
Abacus pass, optional
  → Negotiation pass
  → Post-optimisation
```

相关源码：

- [`../src/dpl/src/NegotiationLegalizer.cpp`](../src/dpl/src/NegotiationLegalizer.cpp)
- [`../src/dpl/src/NegotiationLegalizerPass.cpp`](../src/dpl/src/NegotiationLegalizerPass.cpp)

#### 4.1 Abacus Pass

Abacus 是经典 standard-cell legalization 算法。

思想是：

```text
按 row 扫描 cell
  → 如果 cell 重叠，则合并成 cluster
  → 对 cluster 求最小 displacement 位置
  → collapse cluster，直到 row 内合法
```

OpenROAD 当前 negotiation legalizer 中的 Abacus pass 是可选的：

```tcl
detailed_placement -use_negotiation -abacus
```

#### 4.2 Negotiation Pass

Negotiation pass 类似 PathFinder 风格的协商式资源竞争：

```text
illegal cells + 邻近 cells 作为 active set
  → rip-up 当前放置
  → 搜索新位置
  → 根据 cost 选择位置
  → 对长期拥挤位置增加 history cost
  → 重复直到 violation 降低
```

相关关键函数包括：

- `runNegotiation()`
- `negotiationIter()`
- `ripUp()`
- `place()`
- `findBestLocation()`
- `negotiationCost()`
- `updateHistoryCosts()`
- `greedyImprove()`
- `cellSwap()`

它比 diamond search 更复杂，目标是在混合高度 cell、fence region、局部拥挤等更困难场景下获得更鲁棒的 legalized placement。

---

## 六、Filler Placement 与 Placement Check

`dpl` 还负责 filler cell 放置和 placement 合法性检查。

### 1. Filler Placement

命令：

```tcl
filler_placement FILL*
```

作用：

```text
填充 standard cell row 中的空白 site
  → 连接 VDD / VSS power rail
  → 保持 row 连续性
```

### 2. Check Placement

命令：

```tcl
check_placement
```

作用是检查最终 placement 是否满足基本合法性条件，例如：

- cell 是否在 row 上
- site alignment 是否正确
- 是否有 overlap
- fixed object / blockage 是否冲突
- fence region 是否满足

---

## 七、与 `placement_overview.md` 的对应关系

[`placement_overview.md`](./placement_overview.md) 中提到的现代布局三阶段：

```text
Global Placement
  → Legalization
  → Detailed Placement
```

在 OpenROAD 当前源码中大致对应：

```text
gpl::Replace / NesterovPlace
  → dpl::Opendp / diamondDPL 或 NegotiationLegalizer
  → dpl::Optdp / improve_placement / filler / check
```

如果考虑 macro：

```text
mpl::HierRTLMP
  → gpl::RePlAce-style global placement
  → dpl::OpenDP legalization and detailed placement
```

---

## 八、核心算法总结

OpenROAD 当前 place 算法可以从优化问题角度理解为三类问题的组合。

### 1. Macro Placement：组合优化 / Floorplanning

```text
对象：macro / cluster
变量：macro 位置、方向、cluster shape
目标：wirelength、outline、area、guidance、fence、notch 等
解法：hierarchical clustering + simulated annealing + snapping / pushing
模块：mpl
```

### 2. Global Placement：连续非线性优化

```text
对象：standard cells / movable instances
变量：连续坐标 x, y
目标：smooth wirelength + density penalty
解法：initial BiCGSTAB + Nesterov accelerated gradient descent
模块：gpl
```

### 3. Legalization / Detailed Placement：离散合法化与局部搜索

```text
对象：standard cells
变量：row / site 上的离散合法位置
目标：消除 overlap，最小化 displacement，尽量保持 HPWL
解法：diamond search / Abacus / negotiation-based rip-up and replace
模块：dpl
```

可以浓缩成一句话：

> OpenROAD 的当前 placement 主线是：macro 用层次化模拟退火做 floorplanning，standard cell global placement 用 RePlAce 风格的静电场非线性优化，legalization / detailed placement 用 OpenDP 的 diamond search 或 negotiation legalizer 把连续解落到合法 row/site 上。

---

## 九、建议阅读源码顺序

如果你想真正读懂 OpenROAD placement，建议按以下顺序阅读：

1. [`../src/gpl/README.md`](../src/gpl/README.md)：先理解 `global_placement` 支持什么。
2. [`../src/gpl/src/replace.cpp`](../src/gpl/src/replace.cpp)：看 `doPlace()`、`doInitialPlace()`、`doNesterovPlace()`。
3. [`../src/gpl/src/nesterovPlace.cpp`](../src/gpl/src/nesterovPlace.cpp)：看 Nesterov 主循环。
4. [`../src/gpl/src/nesterovBase.cpp`](../src/gpl/src/nesterovBase.cpp)：看 density、bin、GCell、gradient 计算。
5. [`../src/gpl/src/routeBase.cpp`](../src/gpl/src/routeBase.cpp)：看 routability-driven cell inflation。
6. [`../src/gpl/src/timingBase.cpp`](../src/gpl/src/timingBase.cpp)：看 timing-driven net reweight。
7. [`../src/dpl/README.md`](../src/dpl/README.md)：理解 detailed placement 的两个 engine。
8. [`../src/dpl/src/Opendp.cpp`](../src/dpl/src/Opendp.cpp)：看 detailed placement 主入口。
9. [`../src/dpl/src/Place.cpp`](../src/dpl/src/Place.cpp)：看 diamond search legalization。
10. [`../src/dpl/src/NegotiationLegalizer.cpp`](../src/dpl/src/NegotiationLegalizer.cpp)：看 optional negotiation legalizer。
11. [`../src/mpl/README.md`](../src/mpl/README.md)：如果关心 macro placement，再进入 `mpl`。

---

## 十、最终 Big Picture

从 EDA placement 的角度看，OpenROAD 把一个极其复杂的物理设计问题拆成了多种优化问题：

```text
macro placement
  = 层次化 floorplanning + 组合优化

standard-cell global placement
  = 连续可微非线性优化

legalization / detailed placement
  = 离散网格合法化 + 局部搜索

routability-driven placement
  = 在 placement 优化中注入 routing congestion 反馈

timing-driven placement
  = 在 placement 优化中注入 STA slack 反馈
```

这也解释了为什么现代 placement 工具不是一个单一算法，而是一套混合优化系统：

```text
图论 / 聚类
  + 连续优化
  + 数值线性代数
  + 启发式搜索
  + timing analysis
  + routing congestion estimation
  + 工程合法性检查
```
