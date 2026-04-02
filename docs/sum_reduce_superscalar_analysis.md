# sum_reduce: superscalar 慢于 naive 的组合效应分析

## 1. 问题陈述（Problem Statement）

在 `sum_reduce` 基准中，曾观察到 `sum_superscalar` 在多个问题规模上慢于 `sum_naive`。  
这并不直接意味着“超标量（Superscalar）思路错误”，而是由多个因素叠加导致：

1. 动态分配（Dynamic Allocation）被计入热路径。
2. 运行期 lane（Runtime Lane Count）限制编译器优化。
3. 非 Release 构建（Non-Release Optimization Level）或轻采样（Light Sampling）放大固定开销和抖动。

这三个因素共同作用时，会把本应属于“计算核心”的收益掩盖掉。

## 2. 证据链（Evidence）

### 2.1 实现证据：每次调用都分配 lanes

在 `src/domain/sum_reduce/sum_superscalar.cpp` 中，旧实现每次 `sum_superscalar_impl` 都执行：

```cpp
std::vector<double> lanes(lane_count, 0.0);
```

这意味着一次 `run_once()` 不仅做求和，还隐含了堆分配/释放与初始化成本。  
而 `sum_naive` 只有一个标量累加链，热路径非常短。

### 2.2 编译器优化证据：lane_count 为运行期参数

旧实现的内核结构是“外层按 `lane_count` 步进 + 内层按 `lane` 遍历”。  
当 `lane_count` 在运行期才确定时，编译器难以充分做：

1. 常量传播（Constant Propagation）
2. 循环展开（Loop Unrolling）
3. 寄存器分配稳定化（Register Allocation Stabilization）

结果是多路累加器更容易退化为“内存读改写”，而非寄存器中的独立累加链。

### 2.3 构建与实验配置证据

1. `Debug` 配置下优化等级低（如 `/Od`），会显著削弱手工 ILP 结构。
2. 默认 `run-counts` 与 `warmup` 较轻时，固定开销占比更高，样本抖动对结果影响更大。
3. 计时器按“完整任务”计时，本意正确，但会把分配成本一并纳入算法时间。

## 3. 机制解释（Why Combination Effect Happens）

### 3.1 单因素不足以解释全部现象

1. 若只有动态分配，但编译器优化强、工作量足够大，计算收益有机会盖过分配成本。
2. 若只有运行期 lane，但无分配且数据规模大，仍可能获得部分并行收益。
3. 若只有轻采样，但内核极其稳定，也未必出现系统性反转。

### 3.2 组合后出现“系统性反转”

当三者叠加时，现象会变成：

1. 每次调用的固定成本升高（动态分配）。
2. 计算核心无法被充分优化（运行期 lane）。
3. 样本点对固定成本和抖动更敏感（非 Release 或轻采样）。

于是基准指标反映的是“实现形态惩罚”，而不再是“算法思想收益”。

## 4. 工程结论（Engineering Conclusion）

问题本质不是“超标量归约不可行”，而是旧实现把不必要的运行期开销和优化障碍放在了热路径。

要得到符合预期的对比，应做到：

1. 热路径去分配化（No Allocation on Hot Path）。
2. 常见 lane 走编译期特化（Compile-Time Specialization）。
3. 基准在 Release 优化等级下运行，并提高采样稳健性（更合理 warmup / run-counts）。

## 5. 风险与边界（Risks and Limits）

1. 在极端 lane（非常大且非常规）下，仍可能退回泛化路径，性能不如常见 lane 特化路径。
2. 浮点求和顺序改变会带来微小数值差异（Floating-Point Non-Associativity），但通常在误差容忍范围内。
3. 若未来引入 SIMD Intrinsics（如 AVX2/AVX-512），需重新评估与当前实现的边界和维护成本。

## 6. 后续建议（Next Steps）

1. 用 Release + 统一 CPU 频率策略重新采集 `sum_reduce` 全量数据。
2. 在可行条件下补充性能计数器（Performance Counters）验证瓶颈归因（如 uops、L1 miss、branch）。
3. 针对 `lane=2/4/8/16` 输出单独曲线，避免把“泛化路径”与“特化路径”混在同一解释中。


---

已完成两个任务，并已验证。

**已完成内容**
1. 新增分析文档：  
[docs/sum_reduce_superscalar_analysis.md](/d:/Code/cpu-lab/docs/sum_reduce_superscalar_analysis.md)

2. 修复 `sum_superscalar` 实现层两个问题：  
[sum_superscalar.cpp](/d:/Code/cpu-lab/src/domain/sum_reduce/sum_superscalar.cpp)
- 解决“每次动态分配”：
  - 新增固定大小栈缓冲与泛化路径分离，常见场景不再每次堆分配。
  - 关键位置：`kStackLaneLimit` 与 runtime kernel 路径  
    [sum_superscalar.cpp:145](/d:/Code/cpu-lab/src/domain/sum_reduce/sum_superscalar.cpp:145)
- 解决“运行期 lane 导致优化受限”：
  - 新增编译期特化快路径 `1/2/4/8/16`，让编译器可做更充分展开与寄存器优化。
  - 关键位置：`sum_superscalar_fixed`、`switch(lane_count)`  
    [sum_superscalar.cpp:68](/d:/Code/cpu-lab/src/domain/sum_reduce/sum_superscalar.cpp:68)  
    [sum_superscalar.cpp:167](/d:/Code/cpu-lab/src/domain/sum_reduce/sum_superscalar.cpp:167)

**验证结果**
- 测试通过：
  - `sum_reduce_test`
  - `sum_generator_test`
  - `sum_reduce_benchmark_pipeline_test`
- Release 基准回归（`32,64,128` + warmup `3`）显示 superscalar 已整体快于 naive：  
  - lane 2: 平均 ratio ≈ `0.501`
  - lane 4: 平均 ratio ≈ `0.320`
  - lane 8: 平均 ratio ≈ `0.292`
  - lane 16: 平均 ratio ≈ `0.265`  
  （`ratio = superscalar_best_ns_per_run / naive_best_ns_per_run`，小于 1 表示 superscalar 更快）

**产物文件**
- 新文档：`docs/sum_reduce_superscalar_analysis.md`
- 修改源码：`src/domain/sum_reduce/sum_superscalar.cpp`
- 运行时生成（未提交）：  
  - `data/raw/sum_reduce/benchmark_results_after_fix.csv`
  - `data/raw/sum_reduce/benchmark_results_probe.csv`
  - `data/raw/sum_reduce/benchmark_results_probe_release.csv`
