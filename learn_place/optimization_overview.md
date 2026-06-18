# 最优化（Optimization）概览

欢迎来到最优化（Optimization）的世界！这是一个极具魅力且极其硬核的数学分支。从你之前接触的 EDA 布局布线，到现代的深度学习训练、金融投资组合，甚至是外卖小哥的路线规划，底层全都是最优化问题。

最优化领域的泰斗 Stephen Boyd 曾说过一句名言：**最优化领域真正的分水岭不是线性和非线性，而是凸（Convex）与非凸（Non-convex）。**

为了帮你快速建立全局观，下面是一份从问题定义、核心分类到经典解法，再到权威参考来源的体系大纲。

---

## 一、最优化的标准数学模型

在讨论分类之前，所有的最优化问题都可以抽象为以下这个标准形式：

$$
\begin{aligned}
\min_{x} \quad & f_0(x) \\
\text{s.t.} \quad & f_i(x) \le 0, \quad i = 1, \dots, m \\
& h_i(x) = 0, \quad i = 1, \dots, p
\end{aligned}
$$

- $x$：**决策变量（Decision Variables）**，即我们需要寻找的最优解。
- $f_0(x)$：**目标函数（Objective Function）**，即我们要最小化或最大化的指标，例如线长最短、成本最低。
- $f_i(x)$ 和 $h_i(x)$：**约束条件（Constraints）**，即解必须满足的物理或逻辑规则，例如不能越界、资源有限。

---

## 二、最优化问题的四大核心分类与解法

我们通常根据**目标函数和约束条件的性质**，或者**变量的类型**来对问题进行分类。不同的分类对应着完全不同的解法。

### 1. 按照“凸性”分类（现代最优化的核心视角）

这是目前学术界最看重的分类方式。

#### 凸优化（Convex Optimization）

- **特点**：目标函数是碗状的凸函数，且可行域是凸集。它的核心优势是：**局部最优解一定等于全局最优解**。也就是说，只要你往谷底走，找到的坑就绝对是世上最深的坑。
- **常见解法**：内点法（Interior-point Methods）。
- **现状**：这类问题被认为是“已经彻底解决”的问题。只要问题能转化为凸优化，就可以直接交给商业求解器，例如 Gurobi、MOSEK，或开源库，例如 CVXPY。

#### 非凸优化（Non-convex Optimization）

- **特点**：地形像连绵起伏的山脉，充满了无数个“局部最优解”陷阱。深度学习训练、EDA 布局本质上都是非凸优化。
- **常见解法**：
  - 基于梯度的局部搜索，例如 SGD、Adam。
  - 全局优化启发式算法，例如模拟退火、遗传算法、粒子群算法。
  - 将其松弛（Relaxation）为凸优化问题来求解近似下界。

### 2. 按照“函数线性程度”分类（传统的分类方式）

#### 线性规划（Linear Programming, LP）

- **特点**：目标函数和所有约束条件全是线性的一次方形式。
- **经典解法**：单纯形法（Simplex Method）、内点法。

#### 非线性规划（Non-linear Programming, NLP）

- **特点**：包含二次及以上的高阶项、指数、对数等。
- **无约束非线性解法**：梯度下降（Gradient Descent）、牛顿法（Newton's Method）、共轭梯度法（CG）、拟牛顿法（BFGS / L-BFGS）。
- **有约束非线性解法**：拉格朗日乘子法及 KKT 条件、罚函数法（Penalty Method）、序列二次规划（SQP）。

### 3. 按照“变量类型”分类

#### 连续优化（Continuous Optimization）

- **特点**：变量 $x$ 可以在实数空间 $\mathbb{R}^n$ 内平滑取值，例如坐标、权重。前面提到的梯度下降类方法全都是针对连续优化的。

#### 离散优化 / 组合优化（Discrete / Combinatorial Optimization）

- **特点**：变量 $x$ 只能取整数或 $0/1$。著名的旅行商问题（TSP）、EDA 中的布线问题（Routing）都属于这一类。它通常是 NP-Hard 的。
- **经典解法**：分支定界法（Branch and Bound）、割平面法（Cutting Plane）、动态规划（Dynamic Programming）。整数线性规划（MILP）是其核心工具。

### 4. 按照“确定性”分类

- **确定性优化（Deterministic Optimization）**：所有参数都是已知的常数。
- **随机优化（Stochastic Optimization）**：数据存在随机噪声或概率分布。例如，机器学习中用 mini-batch 计算出的随机梯度，对应的解法是 SGD。

---

## 三、权威参考来源与学习路线（The Bibles of Optimization）

要系统学习优化，建议直接啃这个领域最顶级的“圣经”教材，它们被全世界的顶尖高校作为标准教材。

### 1. 连续优化与算法底层实现（必读经典）

- **书名**：《Numerical Optimization》（数值最优化）
- **作者**：Jorge Nocedal & Stephen J. Wright
- **点评**：如果你想了解梯度下降、牛顿法、L-BFGS、共轭梯度法等算法背后的数学原理和代码实现细节，**这本书是毫无争议的世界第一**。它也是各种现代机器学习框架底层优化器的理论来源。

### 2. 凸优化理论（建立优化思维的神作）

- **书名**：《Convex Optimization》（凸优化）
- **作者**：Stephen Boyd & Lieven Vandenberghe
- **点评**：斯坦福大学 Boyd 教授的封神之作。它教你如何将各种现实生活和工程中的复杂问题，巧妙地通过数学变换“捏”成凸优化模型。
- **配套资源**：斯坦福公开课 **EE364A**，B 站有全套带字幕视频。

### 3. 组合优化与图论

- **书名**：《Combinatorial Optimization: Algorithms and Complexity》
- **作者**：Christos H. Papadimitriou & Kenneth Steiglitz
- **点评**：如果你对 Routing 等基于离散图论的最优化感兴趣，这本书详细推导了从最短路径、网络流到 NP-Complete 问题的经典算法。

---

最优化是一个庞大的家族，不同的问题形态决定了你手中的武器库。你是想先从偏向连续微积分的**非线性规划，例如去搞懂 L-BFGS**开始，还是想从偏向离散图论的**组合优化**入手呢？
