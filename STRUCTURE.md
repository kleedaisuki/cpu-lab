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
我们使用领域驱动设计、C++20。

```g++ -v
Using built-in specs.
COLLECT_GCC=D:\Program Files\msys2\mingw64\bin\g++.exe
COLLECT_LTO_WRAPPER=D:/Program\ Files/msys2/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/15.2.0/lto-wrapper.exe
Target: x86_64-w64-mingw32
Configured with: ../gcc-15.2.0/configure --prefix=/mingw64 --with-local-prefix=/mingw64/local --with-native-system-header-dir=/mingw64/include --libexecdir=/mingw64/lib --enable-bootstrap --enable-checking=release --with-arch=nocona --with-tune=generic --enable-mingw-wildcard --enable-languages=c,lto,c++,fortran,ada,objc,obj-c++,jit --enable-shared --enable-static --enable-libatomic --enable-threads=posix --enable-graphite --enable-fully-dynamic-string --enable-libstdcxx-backtrace=yes --enable-libstdcxx-filesystem-ts --enable-libstdcxx-time --disable-libstdcxx-pch --enable-lto --enable-libgomp --disable-libssp --disable-multilib --disable-rpath --disable-win32-registry --disable-nls --disable-werror --disable-symvers --with-libiconv --with-system-zlib --with-gmp=/mingw64 --with-mpfr=/mingw64 --with-mpc=/mingw64 --with-isl=/mingw64 --with-pkgversion='Rev8, Built by MSYS2 project' --with-bugurl=https://github.com/msys2/MINGW-packages/issues --with-gnu-as --with-gnu-ld --with-libstdcxx-zoneinfo=yes --disable-libstdcxx-debug --enable-plugin --with-boot-ldflags=-static-libstdc++ --with-stage1-ldflags=-static-libstdc++
Thread model: posix
Supported LTO compression algorithms: zlib zstd
gcc version 15.2.0 (Rev8, Built by MSYS2 project)
```

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
├── config/
│   ├── benchmark_matrix.csv # 实验 1 实验配置，每一行代表一个实验
│   ├── benchmark_sum.csv # 实验 2 实验配置，每一行代表一个实验
│   └── plot_config.json # matplotlib 可视化配置
│
├── data/
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
│   │   │   ├── row.hpp # RowLike concept，要求 T::meta() 实现 tuple-like reflection
│   │   │   ├── csv_reader.hpp # 从 csv 加载 Row
│   │   │   └── csv_writer.hpp # 将 Row 写入 csv，支持原子写和批量提交
│   │   └── system/
│   │       └── platform_info.hpp # 获取平台信息或调整配置
│   │
│   ├── domain/
│   │   ├── shared/
│   │   │   ├── benchmark_result.hpp # 实验结果的 RowLike 实现
│   │   │   └── algorithm_orchestrator.hpp # 提供一个备有 operator() 的实验框架，用模板参数解耦实验算法 policy
│   │   │
│   │   ├── matrix_dot/
│   │   │   ├── matrix.hpp # Matrix
│   │   │   ├── vector.hpp # Vector
│   │   │   ├── matrix_generator.hpp # 测试用例生成器 
│   │   │   ├── matrix_dot_naive.hpp # algorithm policy
│   │   │   ├── matrix_dot_cache.hpp # algorithm policy
│   │   │   ├── matrix_dot_cuda_stub.cpp # algorithm policy (CUDA unavailable)
│   │   │   └── matrix_dot_cuda.cu # algorithm policy
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
│           ├── validation_service.hpp # 校验配置正确性
│           └── command_dispatcher.hpp # 分发命令行参数到应用层实现的状态机
│
├── src/
│   │
│   ├── infrastructure/
│   │   ├── timing/
│   │   │   └── high_resolution_timer.cpp # 计算最小二乘功能的高精度计时器
│   │   ├── csv/
│   │   │   ├── row.cpp # RowLike concept，要求 T::meta() 实现 tuple-like reflection
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
│   │   │   ├── matrix_dot_cache.cpp # algorithm policy
│   │   │   └── matrix_dot_cuda.cu # algorithm policy
│   │   │
│   │   └── sum_reduce/
│   │       ├── sum_generator.cpp # 测试用例生成器 
│   │       ├── sum_naive.cpp # algorithm policy
│   │       └── sum_superscalar.cpp # algorithm policy
│   │
│   ├── application/
│   │   ├── matrix_benchmark_pipeline.cpp
│   │   └── sum_reduce_benchmark_pipeline.cpp
│   │
│   └── interfaces/
│       └── cli/
│           ├── cli_parser.cpp # 解析命令行参数
│           ├── command_dispatcher.cpp # 分发命令行参数到应用层实现的状态机
│           ├── validation_service.cpp # 校验配置正确性
│           └── main.cpp # 入口
│
├── scripts/
│   └── visualizer/
│       ├── main.py
│       ├── preprocess.py
│       ├── plotters.py
│       └── requirements.txt
│ 
├── bin/
├── build/
│
├── docs/
│   └── sum_reduce_superscalar_analysis.md
│
├── tex/
│   ├── main.tex
│   ├── preamble.tex
│   │
│   ├── sections/
│   │   ├── index.tex
│   │   ├── 01_architecture/
│   │   │   ├── 01_ddd.tex
│   │   │   ├── 02_infrastructure.tex
│   │   │   ├── 03_domain.tex
│   │   │   └── 04_application_and_interfaces.tex
│   │   ├── 02_algorithm_policies/
│   │   │   ├── 01_matrix_dot_naive.tex
│   │   │   ├── 02_matrix_dot_cache.tex
│   │   │   ├── 03_matrix_dot_cuda.tex
│   │   │   ├── 04_sum_naive.tex
│   │   │   └── 05_sum_superscalar.tex
│   │   ├── 03_experiments/
│   │   │   ├── 01_matrix_dot.tex
│   │   │   └── 02_sum_reduce.tex
│   │   ├── 04_superscalar_winding_exploration/
│   │   │   ├── 01_sum_anomaly_slowdown.tex
│   │   │   ├── 02_why_combination_effect_happens.tex
│   │   │   └── 03_solution.tex
│   │   ├── 05_agents_build_systems/
│   │   │   ├── 01_codex.tex
│   │   │   ├── 02_context_engineering.tex
│   │   │   └── 03_human_in_the_loop.tex
│   ├── figures/
│   │   ├── 03_experiments/
│   │   │   ├── 01_matrix_dot/
│   │   │   │   ├── matrix_normalized_curve.png
│   │   │   │   └── matrix_speedup_heatmap.png
│   │   │   └── 02_sum_reduce/
│   │   │       ├── sum_normalized_curve.png
│   │   │       └── sum_speedup_heatmap.png
│   │   └── 04_superscalar_winding_exploration/
│   │       └── 01_sum_anomaly_slowdown/
│   │           ├── sum_anomaly_slowdown.png
│   │           └── sum_normalized_curve.png
│   ├── tables/
│   │   ├── 01_architecture
│   │   │   └── 01_ddd_layers_and_functions.tex
│   │   ├── 02_algorithm_policies
│   │   │   └── algorithm_policies_overview.tex
│   │   ├── 03_experiments
│   │   │   ├── 01_sum_reduce_before_fix_results.tex
│   │   │   ├── 02_sum_reduce_after_fix_results.tex
│   │   │   └── 03_matrix_dot_results.tex
│   │   └── 05_agents_build_systems/
│   │       └── 01_differences_between_chatbot_and_agent.tex
│   └── latexmkrc
└── tests/
```

实验一、二的 TestCase 以及 BenchmarkOutput 均需要满足 `row.hpp` 中的 `RowLike` 实现 `meta()` 方法来实现 reflection。

`*.hpp` 文件仅含模板、接口和声明，不包含类的实现；实现放在对应 `*.cpp` 中。
