#pragma once

#include "application/benchmark_pipeline.hpp"

namespace cpu_lab::application
{

    /**
     * @brief sum_reduce Pipeline 配置（sum-reduce pipeline config）；Configuration for sum-reduce benchmark pipeline.
     */
    struct SumReduceBenchmarkPipelineConfig
    {
        /** @brief 通用配置（common config）；Shared pipeline config fields. */
        BenchmarkPipelineConfig common{};

        /** @brief 基准名称（benchmark name）；Benchmark name written into result rows. */
        std::string benchmark_name{"sum_reduce"};
    };

    /**
     * @brief sum_reduce Pipeline（sum-reduce pipeline）；CSV-driven sum-reduce benchmark workflow.
     */
    class SumReduceBenchmarkPipeline final : public BenchmarkPipeline
    {
    public:
        /**
         * @brief 构造函数（constructor）；Create pipeline from config.
         * @param config Pipeline 配置（pipeline config）。
         */
        explicit SumReduceBenchmarkPipeline(SumReduceBenchmarkPipelineConfig config);

        /**
         * @brief 执行 pipeline（run pipeline）；Run full sum-reduce workflow and persist result CSV.
         * @return 写出的结果行（written result rows）。
         */
        [[nodiscard]] std::vector<domain::shared::BenchmarkResult> run() const override;

        /**
         * @brief 配置校验（validate config）；Check whether configuration satisfies minimal constraints.
         * @param config Pipeline 配置（pipeline config）。
         * @return 合法返回 true（true if valid）。
         */
        [[nodiscard]] static bool validate_config(const SumReduceBenchmarkPipelineConfig &config) noexcept;

    private:
        /** @brief Pipeline 配置（pipeline config）；Owned runtime configuration. */
        SumReduceBenchmarkPipelineConfig config_{};
    };

} // namespace cpu_lab::application
