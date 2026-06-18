# FastRoute 全局布线算法学习笔记

> 目标：记录 OpenROAD `grt` 模块中 FastRoute 全局布线算法的核心流程、关键数据结构和与 CUGR 的关系。

## 1. grt 模块中的两个全局布线引擎

OpenROAD 的 `src/grt/src` 中包含两个相对独立的全局布线引擎：

1. **FastRoute**
   - 经典全局布线器，也是当前重点学习对象。
   - 采用“先 2D 拥塞优化，再 3D 层分配”的思路。
   - 主要流程在 `FastRouteCore::run()` 中。

2. **CUGR**
   - 另一套替代全局布线器。
   - 更偏向 3D 一体化搜索，使用 pattern routing、稀疏图 maze routing 和 iterative RRR。

`GlobalRouter` 是上层调度器，负责封装和调用这两个引擎。

## 2. 目录结构概览

与 FastRoute / CUGR 相关的核心目录大致如下：

```text
src/grt/src/
├── GlobalRouter.cpp                 # 上层调度器，持有 FastRouteCore 和 CUGR
├── GlobalRouter.tcl                 # Tcl 命令入口
├── Grid.cpp / Grid.h                # grt 外层 grid 抽象
├── Net.cpp / Net.h                  # grt 外层 net 抽象
├── Pin.cpp / Pin.h                  # pin 抽象
├── fastroute/
│   ├── include/
│   │   ├── FastRoute.h              # FastRouteCore 类定义
│   │   ├── DataType.h               # Segment、FrNet、Edge、TreeNode、TreeEdge 等数据结构
│   │   └── Graph2D.h                # 2D 拥塞估计图
│   └── src/
│       ├── FastRoute.cpp            # FastRouteCore::run() 主流程
│       ├── route.cpp                # L / Z / Spiral / Monotonic 等 pattern routing
│       ├── maze.cpp                 # 2D MSMD maze routing
│       ├── maze3D.cpp               # 3D maze routing 相关逻辑
│       ├── RSMT.cpp                 # Steiner tree / RSMT 相关逻辑
│       ├── RipUp.cpp                # rip-up 相关逻辑
│       └── utility.cpp              # layerAssignmentV4 等辅助逻辑
└── cugr/
    ├── include/
    │   └── CUGR.h                   # CUGR 引擎接口
    └── src/
        ├── CUGR.cpp                 # CUGR::route() 主流程
        ├── GridGraph.cpp            # CUGR 的 3D grid graph
        ├── PatternRoute.cpp         # pattern routing + detour
        ├── MazeRoute.cpp            # 稀疏图 maze routing
        └── GRTree.cpp               # Steiner tree 操作
```

FastRoute 的主要代码位置：

- `src/grt/src/fastroute/include/FastRoute.h`
- `src/grt/src/fastroute/include/DataType.h`
- `src/grt/src/fastroute/include/Graph2D.h`
- `src/grt/src/fastroute/src/FastRoute.cpp`
- `src/grt/src/fastroute/src/route.cpp`
- `src/grt/src/fastroute/src/maze.cpp`
- `src/grt/src/fastroute/src/maze3D.cpp`
- `src/grt/src/fastroute/src/utility.cpp`

## 3. FastRoute 的总体思想

FastRoute 可以理解为一个多阶段、迭代式、拥塞驱动的全局布线器。

它的核心思路是：

1. 先为每条 net 生成一个 rectilinear Steiner tree。
2. 将多端 net 拆成多个 2-pin segment。
3. 在 2D grid 上先用简单 pattern route 快速得到初始解。
4. 根据拥塞情况逐步使用更复杂的布线方式：L、Z、Spiral、Monotonic、Maze。
5. 对拥塞 net 进行 rip-up and reroute。
6. 当 2D 路径基本确定后，再做 3D layer assignment，把 2D path 映射到具体金属层。

所以 FastRoute 的主线是：

```text
Pin / Net 数据
  ↓
RSMT / FLUTE
  ↓
2-pin segment decomposition
  ↓
2D pattern routing: L / Z / Spiral / Monotonic
  ↓
2D maze routing: MSMD
  ↓
rip-up and reroute congestion iterations
  ↓
3D layer assignment
  ↓
Global routing guides
```

## 4. FastRoute 主入口：FastRouteCore::run()

FastRoute 的核心主流程位于：

```text
src/grt/src/fastroute/src/FastRoute.cpp
FastRouteCore::run()
```

这个函数是一个多阶段流水线，主要步骤如下。

### 4.1 初始化和预处理

`run()` 开始时会做一些状态清理和技术层预处理：

- 清理 2D graph 的 used 信息。
- 增量模式下重建 used grid。
- 调用 `preProcessTechLayers()` 处理 routing layer 信息。
- 初始化一些 congestion / overflow / cost 相关参数。

这一步为后续 2D/3D routing 准备 grid、capacity、usage、cost 等基础数据。

## 5. 阶段 1：初始 Steiner 树 / RSMT

FastRoute 首先调用：

```cpp
gen_brk_RSMT(false, false, false, false, noADJ);
```

含义：

- 使用 FLUTE 生成 rectilinear Steiner minimal tree。
- 将多端 net 分解成多个 2-pin tree edge / segment。
- 初始阶段还没有强拥塞反馈，因此主要目标是得到较短的初始拓扑。

RSMT 的作用是减少多 pin net 的复杂度。

例如一个 8-pin net 不会直接在 maze 中连接 8 个 pin，而是先生成 Steiner tree，再把 tree edge 当作多个 2-pin routing problem 处理。

## 6. 阶段 2：L 形布线

初始 RSMT 之后，FastRoute 调用：

```cpp
routeLAll(true);
```

对应位置：

```text
src/grt/src/fastroute/src/route.cpp
FastRouteCore::routeLAll()
FastRouteCore::routeSegL()
FastRouteCore::routeSegLFirstTime()
```

### 6.1 L 形路径的两种选择

对于一个 2-pin segment，从 `(x1, y1)` 到 `(x2, y2)`，如果既不共 x，也不共 y，则有两种基本 L 形路径：

```text
路径 A：先竖后横
(x1, y1) → (x1, y2) → (x2, y2)

路径 B：先横后竖
(x1, y1) → (x2, y1) → (x2, y2)
```

FastRoute 会计算两条路径经过边的拥塞代价，选择代价较小的一条。

### 6.2 首次布线的 0.5 / 0.5 估计

在第一次 L routing 前，FastRoute 会对可能路径做估计：

- 如果 segment 是纯水平或纯垂直，直接增加对应边的 estimated usage。
- 如果 segment 是斜对角关系，则把两种 L 路径都按 0.5 权重计入 estimated usage。

这样做的目的是在真正选择 L 方向之前，先建立一个较平滑的拥塞估计。

### 6.3 拥塞驱动的 L routing

之后 FastRoute 会重新调用拥塞驱动版本的 RSMT：

```cpp
gen_brk_RSMT(true, true, true, false, noADJ);
```

此时 RSMT 生成会考虑已有的 2D congestion，使 Steiner tree 尽量避开拥塞区域。

然后再调用：

```cpp
newrouteLAll(false, true);
```

这里 `viaGuided = true`，表示 L routing 时会引入 via 相关代价。

## 7. 阶段 3：Spiral 布线

FastRoute 接着调用：

```cpp
spiralRouteAll();
```

对应位置：

```text
src/grt/src/fastroute/src/route.cpp
FastRouteCore::spiralRouteAll()
```

Spiral routing 的目标是给 pattern routing 更多绕行能力。

L routing 只允许一个拐点；Z routing 通常允许两个拐点；Spiral routing 则可以在局部区域中以类似螺旋扩展的方式尝试绕开拥塞。

可以把它理解为：

```text
L routing      = 最简单、最快，但灵活性最低
Z routing      = 多一个转折点
Spiral routing = 更强的局部绕行能力
Maze routing   = 最通用，但最贵
```

## 8. 阶段 4：Z 形布线

之后 FastRoute 调用：

```cpp
newrouteZAll(10);
```

对应位置：

```text
src/grt/src/fastroute/src/route.cpp
FastRouteCore::newrouteZAll()
```

Z routing 比 L routing 更灵活。

对于一个 2-pin segment，Z route 可以表示为：

```text
HVH: horizontal → vertical → horizontal
VHV: vertical   → horizontal → vertical
```

也就是通过一个中间的 Z point 来绕开拥塞边。

Z routing 仍然属于 pattern routing，不像 maze routing 那样在图上完全自由搜索，因此速度比 maze routing 快。

## 9. 阶段 5：Monotonic Routing

接着进入 monotonic routing：

```cpp
routeMonotonicAll(newTH, enlarge_, logistic_coef);
```

对应位置：

```text
src/grt/src/fastroute/src/route.cpp
FastRouteCore::routeMonotonicAll()
```

Monotonic routing 的特点是路径在总体方向上单调前进，不会来回绕太远。

例如从左下到右上时，路径主要向右和向上扩展。

FastRoute 会多轮调用 monotonic routing，并逐步调整：

- `enlarge_`：搜索窗口大小。
- `newTH`：阈值。
- `logistic_coef`：拥塞代价曲线参数。

这一步仍然比完整 maze routing 便宜，但已经比 L/Z pattern 更灵活。

## 10. 阶段 6：Maze Routing / MSMD

> 详细学习笔记：[FastRoute MSMD Maze Routing 详解](./msmd_maze_routing.md)

FastRoute 中最核心、最通用的路径搜索是：

```cpp
mazeRouteMSMD(...);
```

对应位置：

```text
src/grt/src/fastroute/src/maze.cpp
FastRouteCore::mazeRouteMSMD()
```

MSMD 的含义是：

```text
Multi-Source Multi-Destination
```

即多源多目标迷宫布线。

### 10.1 为什么需要 MSMD

在 Steiner tree 中，一个 tree edge 两端不一定只是单个点。

当 rip-up 某条 tree edge 后，它两侧可能分别对应两个已经连接好的子树。因此重新连接时，起点集合和终点集合都可能有多个 grid point。

所以 maze routing 不是简单的 single-source single-target，而是：

```text
source subtree 的多个 grid point
  → 在 grid graph 上搜索
target subtree 的多个 grid point
```

### 10.2 搜索算法

FastRoute 的 maze routing 本质上是 Dijkstra / A* 风格的最短路径搜索。

它会在 routing grid 上扩展候选点，并根据 cost 选择当前最优点继续扩展。

cost 包括：

- wirelength cost
- congestion cost
- via cost
- history cost
- layer / direction 相关 cost
- NDR 相关 edge cost

### 10.3 Logistic 拥塞代价模型

资料中可以把拥塞代价简化理解为 logistic 形式：

```text
cost = cost_height / (1 + exp(-slope * (usage - capacity)))
```

直观含义：

- 当 `usage < capacity` 时，cost 较低。
- 当 `usage` 接近 `capacity` 时，cost 快速上升。
- 当 `usage > capacity` 时，cost 很高，促使路径避开拥塞边。

实际代码中的 cost 还会结合 history、via、wirelength、NDR 等因素。

## 11. 阶段 7：拥塞迭代 / Rip-up and Re-route

FastRoute 后半段进入核心迭代：

```cpp
while (total_overflow_ > 0 && i <= overflow_iterations_ && ... ) {
    mazeRouteMSMD(...);
    getOverflow2Dmaze(...);
    调整参数;
}
```

这就是经典的 RRR：

```text
Rip-up and Re-route
```

每轮大致流程：

1. 找出拥塞 net / tree edge。
2. rip-up 原有路径，释放 usage。
3. 用当前 cost model 重新 maze route。
4. 更新 edge usage / overflow。
5. 根据结果调整下一轮参数。

### 11.1 自适应参数

FastRoute 在每轮迭代中会动态调整很多参数：

- `enlarge_`：搜索区域扩大程度。
- `costheight_`：拥塞代价高度。
- `slope`：logistic 曲线斜率。
- `logistic_coef`：logistic 相关系数。
- `ripup_threshold`：决定哪些边/网需要 rip-up。
- `mazeedge_threshold_`：maze routing 中边选择阈值。
- `VIA`：via cost 开关或权重。
- `L`：是否允许某些 L 形行为。

这些参数让 FastRoute 在不同拥塞阶段采取不同策略：

```text
严重拥塞：扩大搜索窗、提高拥塞惩罚、减少 via 约束
中等拥塞：稳定推进 rip-up and reroute
轻微拥塞：更精细地局部修复，避免过度扰动
```

### 11.2 收敛检测

FastRoute 不会机械地跑满所有迭代，而是带有自适应收敛检测。

常见退出条件包括：

- `total_overflow_ == 0`
- 达到 `overflow_iterations_`
- overflow 长时间没有改善
- overflow 反复变差超过阈值
- snapshot batch 收敛检测触发
- 达到 maze round 上限

## 12. Snapshot Batch 并行

FastRoute 中有一个比较重要的工程优化：Snapshot Batch Routing。

它的核心思想是：

1. 给 worker 准备 routing state 的快照。
2. 多个 worker 在快照上并行 route 不同 net。
3. 再把结果合并回主状态。

这样可以提升 maze routing 阶段的吞吐量。

但并行 routing 的难点是：

- 多个 net 可能同时选择同一条边。
- 快照中的 congestion 信息可能滞后。
- 合并结果时可能引入新的冲突。

所以代码中还需要额外的 snapshot 收敛检测和低 overflow 阶段的串行 cleanup。

## 13. 3D 层分配：layerAssignmentV4()

FastRoute 的另一个重要特点是 **2D routing 和 3D layer assignment 分离**。

前面的大多数 routing 都是在 2D grid 上解决路径拓扑和拥塞。

之后通过：

```cpp
layerAssignmentV4();
```

对应位置：

```text
src/grt/src/fastroute/src/utility.cpp
FastRouteCore::layerAssignmentV4()
```

把 2D path 分配到具体 metal layer。

Layer assignment 会考虑：

- 每层方向偏好。
- 每层 capacity。
- via cost。
- net 的 min / max routing layer。
- NDR 规则。
- 3D edge usage 和 overflow。

因此 FastRoute 可以概括为：

```text
先在 2D 上决定“走哪里”
再在 3D 中决定“走哪一层”
```

## 14. FastRoute 关键数据结构

主要定义位置：

```text
src/grt/src/fastroute/include/DataType.h
```

### 14.1 Segment

`Segment` 表示一个 2-pin connection。

关键字段：

- `netID`
- `x1, y1, x2, y2`
- `Zpoint`
- `cost`
- `xFirst`
- `HVH`

它是 pattern routing 阶段的基本对象。

### 14.2 FrNet

`FrNet` 是 FastRoute 内部的 net 表示。

它保存：

- 对应的 `odb::dbNet*`
- pin 坐标
- driver index
- min / max routing layer
- edge cost
- slack
- NDR / soft NDR 信息

### 14.3 Edge / Edge3D

`Edge` 表示 2D routing graph 中相邻 grid point 之间的边。

主要字段：

- `cap`：容量
- `usage`：当前使用量
- `red`：capacity reduction
- `real_cap`：未调整前真实容量
- `est_usage`：估计使用量
- `congCNT`：拥塞历史

`Edge3D` 是 3D 版本，按 layer 区分。

### 14.4 TreeNode / TreeEdge

`TreeNode` 和 `TreeEdge` 表示 RSMT / routing tree。

它们连接了：

```text
Steiner tree topology
  ↓
2-pin tree edge
  ↓
routed path grids
```

FastRoute 的 rip-up and reroute 实际上经常是在 tree edge 粒度上进行的。

## 15. Graph2D 的作用

`Graph2D` 是 FastRoute 的 2D 拥塞估计图。

它负责维护：

- horizontal edge usage
- vertical edge usage
- estimated usage
- congestion history
- capacity reduction
- cost 查询和更新

Pattern routing 阶段大量使用 `Graph2D` 来判断某条 L/Z/Spiral 路径是否拥塞。

可以把 `Graph2D` 理解为 FastRoute 早期阶段的“拥塞地图”。

## 16. FastRoute 与 CUGR 对比

虽然当前学习重点是 FastRoute，但理解 CUGR 可以帮助对比两种设计。

| 方面 | FastRoute | CUGR |
| --- | --- | --- |
| 主入口 | `FastRouteCore::run()` | `CUGR::route()` |
| Steiner 树 | FLUTE / RSMT | FLUTE / stt_builder |
| 初始布线 | L / Z / Spiral / Monotonic | Pattern DAG + DP |
| Maze routing | 2D MSMD + 后续 3D layer assignment | 稀疏图上的 3D maze routing |
| 拥塞处理 | 多轮 RRR，复杂自适应参数 | iterative RRR，逐步提高 cost multiplier |
| 层处理 | 2D 和 3D 分离 | 更偏 3D 一体化 |
| 并行 | Snapshot Batch | 主要单线程 |
| NDR | edge cost per layer | computeNdrCosts + Soft-NDR demotion |
| 迭代风格 | 最多很多轮，启发式强 | 阶段更清晰，默认轮数较少 |

一句话总结：

> FastRoute 是一个层分离的 2D → 3D 多阶段布线器；CUGR 是一个更结构化的 3D 一体化 pattern / maze 混合布线器。

## 17. FastRoute 学习路线建议

建议按下面顺序阅读源码：

1. `FastRouteCore::run()`
   - 先理解整体 pipeline。
2. `DataType.h`
   - 理解 `FrNet`、`Segment`、`Edge`、`TreeNode`、`TreeEdge`。
3. `route.cpp`
   - 看 L / Z / Spiral / Monotonic routing 如何基于 congestion cost 选路径。
4. `Graph2D.h / graph2d.cpp`
   - 理解 2D usage、capacity、estimated usage 如何维护。
5. `maze.cpp`
   - 重点看 `mazeRouteMSMD()`，理解 rip-up and reroute 的核心搜索。
6. `maze3D.cpp`
   - 理解 3D routing / layer 相关逻辑。
7. `utility.cpp`
   - 重点看 `layerAssignmentV4()`。
8. 回到 `GlobalRouter.cpp`
   - 理解 FastRoute 如何被 OpenROAD 上层调用、如何生成 guide。

## 18. 核心总结

FastRoute 的核心不是单一算法，而是一套渐进式 routing pipeline：

```text
RSMT
  → L routing
  → congestion-driven RSMT
  → via-guided L routing
  → Spiral routing
  → Z routing
  → Monotonic routing
  → MSMD maze routing
  → iterative rip-up and reroute
  → layer assignment
```

它的关键思想包括：

- 用 FLUTE / RSMT 降低多 pin net 复杂度。
- 用 pattern routing 快速得到初始解。
- 用 congestion-driven cost 引导路径避开热点区域。
- 用 MSMD maze routing 修复复杂拥塞。
- 用 RRR 迭代逐步降低 overflow。
- 用 layer assignment 把 2D 路径映射到 3D 金属层。
- 用 Snapshot Batch 提高大规模 maze routing 的并行效率。

可以把 FastRoute 理解为：

> 一个以拥塞为核心反馈信号、由简单到复杂逐步升级路径搜索能力的全局布线器。
