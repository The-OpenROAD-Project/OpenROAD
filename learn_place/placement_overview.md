# Placement 现代布局算法初步理解

在现代先进制程（如 7nm、5nm、3nm 及以下）中，由于芯片上的标准单元（Standard Cells）数量已经飙升到**数千万甚至数亿个**，传统的布局算法（如基于模拟退火的算法）由于计算复杂度太高，已经完全无法应对。

现代工业界和顶尖学术界的 Placement（布局）工具，其核心算法架构高度一致，几乎完全由解析式布局算法（Analytical Placement）统治。整个现代布局流程通常分为三个阶段：**全局布局（Global Placement）**、**合法化（Legalization）**和**详细布局（Detailed Placement）**。

以下是现代最核心的布局算法和技术框架。

---

## 1. 绝对的主流：基于静电场模型的非线性优化（Electrostatics-based Non-linear Placement）

这是近十年来 EDA 领域最伟大的突破之一，也是现代商用工具（如 Cadence Innovus、Synopsys ICC2）以及顶尖开源工具（OpenROAD 中的 RePlAce）的核心算法。

### 代表算法 / 工具

- **ePlace**：加州大学圣迭戈分校卢静国教授团队提出。
- **RePlAce**：OpenROAD 中全局布局器的重要基础。
- **DREAMPlace**：北京大学 / 德克萨斯大学奥斯汀分校林亦波教授团队提出。

### 核心思想

可以用一个很形象的比喻理解：

- 把芯片上的**标准单元**看作带**正电荷的粒子**。
- 粒子之间会产生排斥力。
- 如果某个地方单元堆积得太密，电荷密度就高，排斥力就会把单元“推开”，从而在数学上解决单元重叠（Overlap）问题。
- 同时，单元之间的**连线**可以看作**弹簧**。
- 线越长，拉力越大，拉力会吸引单元靠拢，从而优化**总线长（HPWL）**。

### 数学本质

[静电场模型将布局问题转化为一个连续可微的无约束非线性优化问题。](./nonlinear_continuous_optimization.md)

核心包括：

- 通过求解泊松方程（Poisson's Equation）计算密度势场。
- 使用 Nesterov's Accelerated Gradient Descent 等梯度优化方法求解大规模连续优化问题。
- 同时优化线长目标和密度目标。

### 颠覆性优势

它可以同时处理超大规模设计中的线长和密度问题，并且天然适合 GPU 加速。

---

## 2. 现代演进：GPU 加速布局算法（GPU-Accelerated Placement）

随着深度学习硬件的发展，EDA 工程师发现：非线性布局算法中的“梯度下降求解线长和密度”，在数学形式上和深度学习训练神经网络非常相似。

### 代表工具

- **DREAMPlace**

DREAMPlace 是近几年 EDA 顶会 ISPD / DAC 中非常有代表性的 GPU 加速布局工具。

### 核心创新

DREAMPlace 将非线性布局算法（ePlace 框架）映射到深度学习框架 PyTorch 中：

- 把芯片单元的坐标看作“网络参数”。
- 把线长和密度函数看作“损失函数（Loss）”。
- 借助 PyTorch 的 GPU 并行计算和自动梯度能力进行优化。

### 结果

DREAMPlace 利用 GPU 并行能力，将原本在 CPU 上需要较长时间的百万级单元布局显著加速，在一些实验中可以达到数十倍加速，极大提升了布局效率。

---

## 3. 经典长青：二次型布局算法（Quadratic / Force-Directed Placement）

在非线性静电场模型流行之前，二次型布局是工业界上一代非常重要的布局方法。即使在今天，许多工具在初始布局（Initial Placement）阶段仍可能使用它来快速得到一个初始解。

### 代表算法 / 工具

- **Gordian**
- **Kraftwerk**
- **mPL**

### 核心思想

二次型布局将连线长度的平方和作为目标函数。

由于二次函数是凸函数，求导后会变成一组线性方程组：

$$
Ax = b
$$

### 求解方法

可以利用成熟的数值方法快速求解：

- 共轭梯度法（CG）
- 代数多网格法（AMG）
- 其他稀疏线性方程组求解器

### 缺点

二次型布局求出的坐标往往会导致严重的单元重叠。

原因是弹簧模型会把互相连接的单元拉向一起，很多单元容易集中到芯片中心或局部区域。

因此，它通常需要配合：

- “切蛋糕”式空间划分（Partitioning）
- 密度力（Density Force）
- 后续 legalization

来逐步把单元推开。

---

## 4. 后端必经之路：合法化与详细布局算法（Legalization & Detailed Placement）

无论是静电场模型还是二次型布局，它们算出来的单元坐标都是连续坐标。

这意味着：

- 单元之间可能仍然有重叠。
- 单元没有完全对齐到标准单元行（Rows）。
- 坐标不一定满足站点（Site）约束。

因此需要后续算法进行合法化和详细优化。

### 4.1 Legalization（合法化算法）

#### 做什么

Legalization 负责：

- 把单元移动到最近的合法 row / site 上。
- 彻底消除单元重叠。
- 尽量最小化单元移动距离（Displacement）。
- 尽量减少对时序和线长的破坏。

#### 常用算法：Abacus

**Abacus** 是非常经典的 legalization 算法。

它采用动态规划（Dynamic Programming）的思想：

- 将单元从左到右放入 row。
- 如果出现重叠，就把相邻单元合并成 cluster。
- 对 cluster 的位置做整体调整。
- 在保证合法的同时尽量接近 global placement 给出的目标位置。

Abacus 的特点是速度快、效果稳定，并且对时序破坏相对较小。

### 4.2 Detailed Placement（详细布局优化算法）

#### 做什么

Detailed Placement 在已经合法的布局上做局部微调，目标是进一步优化：

- HPWL
- timing
- congestion
- cell displacement
- local density

#### 常用算法：窗口滑动优化（Window-based Optimization）

窗口滑动优化的思想是：

- 每次框选一小块区域，例如若干个标准单元或若干个 row segment。
- 在窗口内部尝试单元重排、交换、移动。
- 使用 global matching、branch and bound 或启发式搜索找到更优排列。
- 然后像滑动窗口一样遍历整个芯片。

这种方法可以在不破坏全局合法性的前提下，持续压榨线长和时序。

---

## 5. 现代先进制程下的硬核挑战：考虑物理约束的布局

在 5nm 及以下，布局算法不能只看“线长”和“密度”，还必须考虑更多物理约束。

### 5.1 Timing-driven Placement（时序驱动布局）

时序驱动布局会在优化目标中引入 STA 信息。

典型做法是：

- 根据 STA 传回的 slack 判断关键路径。
- 给关键路径上的 net 或 cell 加更大的权重。
- 等价于加强关键路径上单元之间的“弹簧拉力”。
- 迫使关键单元靠得更近，从而降低 wire delay。

### 5.2 Routability-driven Placement（可布线性驱动布局）

有些布局从线长和密度上看很好，但布线阶段会发现局部连线过密，导致 routing congestion。

Routability-driven placement 会在布局过程中引入拥塞预估：

- 预测局部 routing demand。
- 发现 congestion hotspot。
- 将拥塞区域的单元推开。
- 必要时牺牲一部分线长，为布线留出通道。

相关思想包括波前扩散、概率图模型、RUDY 类拥塞估计等。

### 5.3 Pin-Access / Cell Spacing Awareness

先进制程中，pin 非常小，布线规则非常复杂。

如果两个复杂单元挨得太近，虽然布局合法，但 detailed router 可能无法接入 pin。

因此现代布局器需要提前考虑：

- pin access
- cell spacing
- local routing resource
- design rule friendliness

这类约束对 7nm、5nm、3nm 及以下节点非常重要。

---

## 6. 总结：现代布局的“三驾马车”

现代 placement 的典型流程可以概括为：

$$
\text{DREAMPlace/RePlAce 全局非线性静电场求解}
\rightarrow
\text{Abacus 合法化消除重叠}
\rightarrow
\text{OpenDP / Detailed Placement 滑动窗口详细优化}
$$

也可以从工程流程上理解为：

```text
Global Placement
  → Legalization
  → Detailed Placement
```

其中：

- Global Placement 负责全局线长和密度。
- Legalization 负责把连续解变成物理合法解。
- Detailed Placement 负责局部优化和最终质量提升。

这套组合是当前应对数千万到数亿级标准单元物理布局问题的核心方法之一。
