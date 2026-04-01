实验内容：

1. 计算给定n*n矩阵的每一列与给定向量的内积，考虑两种算法设计思路：
    1. 逐列访问元素的平凡算法。
    2. cache优化算法。
2. 计算n个数的和，考虑两种算法设计思路：
    1. 逐个累加的平凡算法（链式）。
    2. 超标量优化算法（指令级并行），如最简单的两路链式累加；再如递归算法——两两相加、中间结果再两两相加，依次类推，直至只剩下最终结果。

要求：
使用高精度计时。
测试数据的生成采用人为设计的固定值即可，便于判断算法正确性。
计时、测试要考虑全面，令结果更有说服力，多次重复计时后采用最小二乘近似。
再如测试不同问题规模，分析其与系统参数（如cache大小）相对关系对性能的影响等。
我们使用领域驱动设计。

```
cpu-lab/
├── README.md
├── LICENSE
│
├── .gitignore
├── STRUCTURE.md
│
├── CMakeLists.txt
│
├── requirements.txt
│
├── config/
│   ├── benchmark_matrix.csv # 实验 1 实验配置，每一行代表一个实验
│   ├── benchmark_sum.csv # 实验 2 实验配置，每一行代表一个实验
│   └── plot_config.json # matplotlib 可视化配置
│
├── data/
│   ├── input/
│   │   ├── matrix/
│   │   └── sum/
│   ├── raw/
│   │   ├── matrix_dot/
│   │   └── sum_reduce/
│   ├── processed/
│   │   ├── summary/
│   │   └── tables/
│   └── figures/
│       ├── matrix_dot/
│       └── sum_reduce/
│
├── include/
│   │
│   ├── infrastructure/
│   │   ├── timing/
│   │   │   └── high_resolution_timer.hpp # 计算最小二乘功能的高精度计时器
│   │   ├── csv/
│   │   │   ├── row.hpp # Row 的 struct 基类，提供 T::meta() 实现 tuple-like reflection
│   │   │   ├── csv_reader.hpp # 从 csv 加载 Row
│   │   │   └── csv_writer.hpp # 将 Row 写入 csv，支持原子写和批量提交
│   │   └── system/
│   │       └── platform_info.hpp # 获取平台信息或调整配置
│   │
│   ├── domain/
│   │   ├── shared/
│   │   │   ├── benchmark_result.hpp # 实验结果的 Row
│   │   │   └── algorithm_orchestrator.hpp # 提供一个备有 operator() 的实验框架，用模板参数解耦实验算法 policy
│   │   │
│   │   ├── matrix_dot/
│   │   │   ├── matrix.hpp # Matrix
│   │   │   ├── vector.hpp # Vector
│   │   │   ├── matrix_generator.hpp # 测试用例生成器 
│   │   │   ├── matrix_dot_naive.hpp # algorithm policy
│   │   │   └── matrix_dot_cache.hpp # algorithm policy
│   │   │
│   │   └── sum_reduce/
│   │       ├── sum_generator.hpp # 测试用例生成器 
│   │       ├── sum_naive.hpp # algorithm policy
│   │       └── sum_superscalar.hpp # algorithm policy
│   │
│   ├── application/
│   │   ├── benchmark_pipeline.hpp # BenchmarkPipeline 基类接口
│   │   ├── matrix_benchmark_pipeline.hpp
│   │   └── sum_reduce_benchmark_pipeline.hpp
│   │
│   └── interfaces/
│       └── cli/
│           ├── cli_parser.hpp # 解析命令行参数
│           └── command_dispatcher.hpp # 分发命令行参数到应用层实现的状态机
│
├── src/
│   │
│   ├── infrastructure/
│   │   ├── timing/
│   │   │   └── high_resolution_timer.cpp # 计算最小二乘功能的高精度计时器
│   │   ├── csv/
│   │   │   ├── row.cpp # Row 的 struct 基类，提供 T::meta() 实现 tuple-like reflection
│   │   │   ├── csv_reader.cpp # 从 csv 加载 Row
│   │   │   └── csv_writer.cpp # 将 Row 写入 csv，带锁
│   │   └── system/
│   │       └── platform_info.cpp # 获取平台信息或调整配置
│   │
│   ├── domain/
│   │   ├── shared/
│   │   │   ├── benchmark_result.cpp # 实验结果的 Row
│   │   │   └── algorithm_orchestrator.cpp # 提供一个写了 operator() 的实验框架，用模板参数解耦实验算法 policy
│   │   │
│   │   ├── matrix_dot/
│   │   │   ├── matrix.cpp # Matrix
│   │   │   ├── vector.cpp # Vector
│   │   │   ├── matrix_generator.cpp # 测试用例生成器 
│   │   │   ├── matrix_dot_naive.cpp # algorithm policy
│   │   │   └── matrix_dot_cache.cpp # algorithm policy
│   │   │
│   │   └── sum_reduce/
│   │       ├── sum_generator.cpp # 测试用例生成器 
│   │       ├── sum_naive.cpp # algorithm policy
│   │       └── sum_superscalar.cpp # algorithm policy
│   │
│   ├── application/
│   │   ├── benchmark_pipeline.cpp # BenchmarkPipeline 基类接口
│   │   ├── matrix_benchmark_pipeline.cpp
│   │   └── sum_reduce_benchmark_pipeline.cpp
│   │
│   └── interfaces/
│       └── cli/
│           ├── cli_parser.cpp # 解析命令行参数
│           ├── command_dispatcher.cpp # 分发命令行参数到应用层实现的状态机
│           └── main.cpp # 入口
│
├── scripts/
│   └── visualizer/
│       ├── main.py
│       ├── preprocess_results.py
│       ├── plot_matrix_dot.py
│       ├── plot_sum_reduce.py
│       ├── export_tables.py
│       └── plot_style.py
│ 
├── bin/
├── build/
│
├── tex/
│   ├── main.tex
│   ├── sections/
│   ├── figures/
│   ├── tables/
│   ├── references.bib
│   └── latexmkrc
└── tests/
```

实验一、二的 TestCase 以及 BenchmarkOutput 均需要继承 `row.hpp` 中的 `Row` 实现 `meta()` 方法来实现 reflection。

`*.hpp` 文件仅含接口和声明，不包含类的实现；实现放在对应 `*.cpp` 中。
