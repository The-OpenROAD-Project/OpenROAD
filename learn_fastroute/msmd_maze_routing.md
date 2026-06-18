# FastRoute MSMD Maze Routing 详解

MSMD 是 FastRoute 中 2D maze routing 阶段的核心算法，全称是：

```text
Multi-Source Multi-Destination
```

也就是 **多源多目标迷宫布线**。

对应源码位置：

- `src/grt/src/fastroute/src/maze.cpp`
- `FastRouteCore::mazeRouteMSMD()`
- `FastRouteCore::mazeRouteMSMDSequential()`
- `FastRouteCore::setupHeap()`

---

## 1. MSMD 要解决什么问题？

普通 maze routing 通常是：

```text
一个 source 点 → 一个 target 点
```

但 FastRoute 不是直接对两个 pin 做点到点布线，而是在 Steiner tree 上做 rip-up and reroute。

对于一个多 pin net，FastRoute 已经有一棵 routing tree。当它想重布某条 tree edge `(n1, n2)` 时，如果把这条 edge 拆掉，整棵树会被分成两边：

```text
subtree 1  —— 被拆掉的 edge ——  subtree 2
```

重新连接时，不一定要从原来的 `n1` 连接到 `n2`。

更好的路径可能是：

```text
subtree 1 上的某个已有 grid point
连接到
subtree 2 上的某个已有 grid point
```

所以 MSMD 的核心是：

```text
多个 source 点
  → 在 grid graph 上搜索
多个 destination 点
```

这比普通 2-pin maze routing 灵活很多，尤其适合多 pin net 的拥塞修复。

---

## 2. 顶层入口

入口函数是：

```cpp
void FastRouteCore::mazeRouteMSMD(...)
{
  if (!runSnapshotBatchedMazeRoute(...)) {
    mazeRouteMSMDSequential(...);
  }
}
```

它的逻辑是：

1. 如果当前 iteration 适合 snapshot batch routing，就走并行版本。
2. 否则走普通顺序版本 `mazeRouteMSMDSequential()`。

Snapshot batch 是工程加速手段，不改变 MSMD 的核心搜索思想。

真正的搜索逻辑主要在：

```cpp
FastRouteCore::mazeRouteMSMDSequential()
```

---

## 3. 预计算 cost table

`mazeRouteMSMDSequential()` 开始时会为水平边和垂直边预计算 cost table：

```cpp
const int max_h_usage = max_usage_multiplier * h_capacity_;
h_cost_table_.reserve(max_h_usage);
for (int i = 0; i < max_h_usage; i++) {
  h_cost_table_.push_back(getCost(i, true, cost_params));
}

const int max_v_usage = max_usage_multiplier * v_capacity_;
v_cost_table_.reserve(max_v_usage);
for (int i = 0; i < max_v_usage; i++) {
  v_cost_table_.push_back(getCost(i, false, cost_params));
}
```

这样可以避免搜索过程中重复计算 logistic cost。

实际 cost 函数是：

```cpp
double cost
    = cost_height / (std::exp((capacity - index) * logistic_coef) + 1) + 1;

if (index >= capacity) {
  cost += (cost_height / slope * (index - capacity));
}
```

这里 `index` 可以理解为当前边的拥塞程度，通常与下面因素有关：

- 当前 usage
- capacity reduction
- history usage

因此：

```text
边越拥塞 → index 越大 → cost 越高
```

当 `index >= capacity` 时，还会额外增加线性惩罚。

---

## 4. Net 和 Tree Edge 的处理顺序

如果 `ordering = true`，FastRoute 会先做 timing / congestion-aware 排序：

```cpp
if (ordering) {
  if (critical_nets_percentage_) {
    slack_th = CalculatePartialSlack();
  }
  StNetOrder();
}
```

这意味着：

- critical net 可能会被优先处理。
- 拥塞更严重的 net / edge 可能会被优先处理。

之后进入两层循环：

```text
for each net:
  for each tree edge in this net:
    判断是否需要 rip-up and reroute
```

对应代码结构：

```cpp
for (int nidRPC = 0; nidRPC < net_ids_.size(); nidRPC++) {
  const int netID
      = ordering ? tree_order_cong_[nidRPC].treeIndex : net_ids_[nidRPC];

  netedgeOrderDec(netID, net_eo);

  for (int edgeREC = 0; edgeREC < num_edges; edgeREC++) {
    const int edgeID = net_eo[edgeREC].edgeID;
    TreeEdge* treeedge = &(treeedges[edgeID]);
    ...
  }
}
```

---

## 5. 判断一条 edge 是否需要 rip-up

不是所有 tree edge 都会进入 maze routing。

首先，太短的 edge 会被跳过：

```cpp
if (treeedge->len <= maze_edge_threshold) {
  continue;
}
```

然后调用：

```cpp
newRipupCheck(...)
```

它大致判断：

- 这条 edge 是否经过拥塞区域。
- 是否超过 rip-up threshold。
- 是否属于 critical net。
- 当前路径是否值得重布。

如果 `newRipupCheck()` 返回 false，则跳过这条 edge：

```cpp
if (!enter) {
  continue;
}
```

---

## 6. 限定局部搜索窗口

FastRoute 不会在整个芯片 grid 上搜索，因为搜索空间太大。

它会围绕当前 tree edge 的 bbox 扩大一个窗口：

```cpp
enlarge_ = std::min(origENG, (iter / 6 + 3) * treeedge->route.routelen);

const int regionX1 = std::max(xmin - enlarge_ + decrease, 0);
const int regionX2 = std::min(xmax + enlarge_ - decrease, x_grid_ - 1);
const int regionY1 = std::max(ymin - enlarge_ + decrease, 0);
const int regionY2 = std::min(ymax + enlarge_ - decrease, y_grid_ - 1);
```

对于 critical net，窗口可能会缩小一些：

```cpp
if (nets_[netID]->isCritical()) {
  decrease = std::min((iter / 7) * 5, enlarge_ / 2);
}
```

直觉是：

```text
critical net 不希望绕太远，因为绕远会恶化 timing。
```

---

## 7. setupHeap()：MSMD 的关键初始化

搜索开始前，FastRoute 调用：

```cpp
setupHeap(netID,
          edgeID,
          src_heap,
          dest_heap,
          d1,
          d2,
          regionX1,
          regionX2,
          regionY1,
          regionY2);
```

### 7.1 2-pin net 情况

如果 net 只有两个 terminal，它退化为普通点到点 maze routing：

```cpp
d1[y1][x1] = 0;
src_heap.push_back(&d1[y1][x1]);

d2[y2][x2] = 0;
dest_heap.push_back(&d2[y2][x2]);
```

此时：

```text
source set = {n1}
destination set = {n2}
```

### 7.2 Multi-pin net 情况

如果是多 pin net，`setupHeap()` 会从 `n1` 一侧遍历 subtree，把 subtree 上已有路径中的 grid point 都加入 source set：

```cpp
d1[y_grid][x_grid] = 0;
src_heap.push_back(&d1[y_grid][x_grid]);
corr_edge_[y_grid][x_grid] = edge;
```

同时，从 `n2` 一侧遍历另一棵 subtree，把那边已有路径中的 grid point 都加入 destination set：

```cpp
d2[y_grid][x_grid] = 0;
dest_heap.push_back(&d2[y_grid][x_grid]);
corr_edge_[y_grid][x_grid] = edge;
```

最终形成：

```text
src_heap  = subtree 1 上所有可接入 grid point
dest_heap = subtree 2 上所有可接入 grid point
```

这就是 MSMD 的核心。

---

## 8. Dijkstra 式搜索

搜索前，FastRoute 会把所有 destination 点标记到 `pop_heap2`：

```cpp
for (auto& dest : dest_heap) {
  pop_heap2[dest - &d2[0][0]] = true;
}
```

然后开始主循环：

```cpp
while (!pop_heap2[ind1]) {
  const int curX = ind1 % x_range_;
  const int curY = ind1 / x_range_;

  removeMin(src_heap);

  relaxAdjacent(curX, curY, -1, 0, ...);
  relaxAdjacent(curX, curY,  1, 0, ...);
  relaxAdjacent(curX, curY,  0,-1, ...);
  relaxAdjacent(curX, curY,  0, 1, ...);

  ind1 = (src_heap[0] - &d1[0][0]);
}
```

含义是：

```text
从所有 source 同时出发。
每次取当前 dist 最小的点。
向上下左右四个方向扩展。
直到当前最小点属于 destination set。
```

这本质上是 **multi-source Dijkstra**。

严格来说，这段实现没有显式 A* heuristic，因此它更像 Dijkstra / maze routing，而不是标准 A*。

---

## 9. relaxAdjacent()：边代价如何计算？

`relaxAdjacent()` 会计算从当前 grid 走到相邻 grid 的新增代价。

核心逻辑：

```cpp
const bool is_horizontal = d_x != 0;

const int pos1 = (graph2d_.*usage_red)(p1_x, p1_y)
                 + L * (graph2d_.*last_usage)(p1_x, p1_y);

double cost1 = getCost(pos1, is_horizontal, cost_params);

double tmp = d1[cur_y][cur_x] + cost1;
```

其中：

- `usage_red`：当前 usage 加上 capacity reduction。
- `last_usage`：上一轮使用情况，体现 history。
- `L`：是否引入历史使用惩罚。
- `getCost()`：把拥塞程度映射成边代价。

如果发生转弯，还会加 via-like penalty：

```cpp
if (add_via && d1[cur_y][cur_x] != 0) {
  tmp += via;
}
```

虽然这里是 2D maze routing，但转弯通常会暗示潜在换层或更复杂实现，所以使用 `via` 作为额外惩罚。

---

## 10. Parent 指针和路径回溯

当某个邻居点被更低 cost 更新时，FastRoute 会记录 parent：

```cpp
if (cur_x != adj_x) {
  parent_x3_[adj_y][adj_x] = cur_x;
  parent_y3_[adj_y][adj_x] = cur_y;
  hv_[adj_y][adj_x] = false;
} else {
  parent_x1_[adj_y][adj_x] = cur_x;
  parent_y1_[adj_y][adj_x] = cur_y;
  hv_[adj_y][adj_x] = true;
}
```

当搜索碰到 destination set 时，当前点就是交汇点：

```cpp
const int16_t crossX = ind1 % x_range_;
const int16_t crossY = ind1 / x_range_;
```

然后从 `(crossX, crossY)` 沿 parent 指针回溯到 source subtree：

```cpp
while (d1[curY][curX] != 0) {
  ...
  tmp_grids.push_back({curX, curY, -1});
}
```

最后反转路径并加入交汇点：

```cpp
std::vector<GPoint3D> grids(tmp_grids.rbegin(), tmp_grids.rend());
grids.push_back({crossX, crossY, -1});
```

这条 `grids` 就是 MSMD 找到的新 2D 路径。

---

## 11. 更新 Steiner Tree 结构

MSMD 找到的新路径不一定连接原来的 `n1` 和 `n2`。

它可能连接的是：

```text
subtree 1 上某个点 E1
到
subtree 2 上某个点 E2
```

代码中会取出：

```cpp
const int E1x = grids[0].x;
const int E1y = grids[0].y;
const int E2x = grids.back().x;
const int E2y = grids.back().y;
```

如果 `E1` 或 `E2` 落在已有 tree edge 的中间，FastRoute 需要调整 tree topology，可能会：

- split edge
- 更新 tree node 坐标
- 更新相邻关系
- 更新原有 route grids

相关函数包括：

```cpp
splitEdge(...)
updateRouteType1(...)
updateRouteType2(...)
```

这部分是 MSMD 比普通 maze routing 更复杂的地方，因为它不只是找一条路径，还要维护 Steiner tree 的合法拓扑。

---

## 12. 写回新 route，并更新 usage

最后，FastRoute 把新路径写回当前 tree edge：

```cpp
treeedges[edge_n1n2].route.grids.assign(cnt_n1n2, GPoint3D{});
treeedges[edge_n1n2].route.type = RouteType::MazeRoute;
treeedges[edge_n1n2].route.routelen = cnt_n1n2 - 1;

treeedges[edge_n1n2].len = abs(E1x - E2x) + abs(E1y - E2y);
```

然后更新 `Graph2D` usage：

```cpp
for (int i = 0; i < cnt_n1n2 - 1; i++) {
  if (grids[i].x == grids[i + 1].x) {
    graph2d_.updateUsageV(...);
  } else {
    graph2d_.updateUsageH(...);
  }
}
```

这一步很关键：

```text
新路径占用了哪些 horizontal / vertical edge
  → 更新 usage
  → 后续 routing 看到这些边更贵
  → 形成拥塞反馈
```

---

## 13. 整体伪代码

```text
for each net:
  按拥塞 / timing 排序 tree edges

  for each tree edge e = (n1, n2):
    if edge 太短:
      continue

    if 不需要 rip-up:
      continue

    确定局部搜索窗口 region

    setupHeap():
      src_heap  = n1 一侧 subtree 上所有可接入 grid
      dest_heap = n2 一侧 subtree 上所有可接入 grid

    while 当前最小 cost 点不属于 dest set:
      从 src_heap 取出 dist 最小点
      对上下左右邻居做 relax
      cost = 拥塞代价 + 转弯/via 代价 + history 代价

    cross point = 第一个碰到的 destination 点

    根据 parent 指针回溯新路径

    调整 Steiner tree 拓扑

    写回 tree edge route

    更新 Graph2D usage
```

---

## 14. 直观例子

假设某个 4-pin net 的当前 routing tree 是：

```text
A ---- S ---- B
       |
       C
       |
       D
```

现在 FastRoute 发现 `S-C` 这条 edge 穿过拥塞区域，于是 rip-up `S-C`。

拆掉后形成两棵 subtree：

```text
subtree 1: A, S, B 以及它们之间已有路径
subtree 2: C, D 以及它们之间已有路径
```

MSMD 不一定非要重新连：

```text
S → C
```

它可能发现更好的连接是：

```text
B 附近某个 grid → D 附近某个 grid
```

这样可以绕开拥塞区域。

---

## 15. 总结

FastRoute 的 MSMD maze routing 可以概括为：

> 对一条被 rip-up 的 Steiner tree edge，不是简单重连两个端点，而是把 edge 两侧的两个 subtree 分别看成 source set 和 destination set，在局部窗口内用 Dijkstra 式 maze routing 找到一条拥塞代价最小的新连接路径，并更新 routing tree 与 2D usage。

它解决的问题是：

```text
普通 2-pin maze routing:
  n1 → n2

MSMD maze routing:
  any point in subtree(n1 side)
    →
  any point in subtree(n2 side)
```

所以它更灵活，也更适合多 pin net 的拥塞修复。
