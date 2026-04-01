#pragma once

#include "domain/shared/benchmark_result.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace cpu_lab::application
{

    /**
     * @brief Pipeline 通用配置（shared pipeline config）；Common input/output and timing options for benchmark pipelines.
     */
    struct BenchmarkPipelineConfig
    {
        /** @brief 输入 CSV 路径（input CSV path）；Path to test-case CSV file. */
        std::string input_csv_path{};

        /** @brief 输出 CSV 路径（output CSV path）；Path to benchmark-result CSV file. */
        std::string output_csv_path{};

        /** @brief 每个采样的运行次数列表（run-count list）；Run counts used by timer sampling. */
        std::vector<std::size_t> run_counts{};

        /** @brief 预热轮数（warmup rounds）；Warmup rounds before timed samples. */
        std::size_t warmup_rounds{1U};

        /** @brief 输出是否追加（append output）；Append rows to existing output file. */
        bool append_output{false};

        /** @brief 输出是否原子写（atomic output）；Use temp-file commit for non-append writes. */
        bool atomic_output{true};
    };

    /**
     * @brief 基准 Pipeline 接口（benchmark pipeline interface）；Runtime-polymorphic pipeline contract.
     */
    class BenchmarkPipeline
    {
    public:
        /**
         * @brief 虚析构函数（virtual destructor）；Enable safe polymorphic destruction.
         */
        virtual ~BenchmarkPipeline() = default;

        /**
         * @brief 执行 pipeline（run pipeline）；Read cases, generate ctor args, orchestrate benchmarks, and write outputs.
         * @return 写出的结果行（written result rows）。
         */
        [[nodiscard]] virtual std::vector<domain::shared::BenchmarkResult> run() const = 0;
    };

} // namespace cpu_lab::application
