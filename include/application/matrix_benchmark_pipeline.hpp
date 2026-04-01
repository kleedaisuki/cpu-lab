#pragma once

#include "application/benchmark_pipeline.hpp"

namespace cpu_lab::application
{

    /**
     * @brief matrix_dot Pipeline 配置（matrix-dot pipeline config）；Configuration for matrix-dot benchmark pipeline.
     */
    struct MatrixBenchmarkPipelineConfig
    {
        /** @brief 通用配置（common config）；Shared pipeline config fields. */
        BenchmarkPipelineConfig common{};

        /** @brief 基准名称（benchmark name）；Benchmark name written into result rows. */
        std::string benchmark_name{"matrix_dot"};
    };

    /**
     * @brief matrix_dot Pipeline（matrix-dot pipeline）；CSV-driven matrix-dot benchmark workflow.
     */
    class MatrixBenchmarkPipeline final : public BenchmarkPipeline
    {
    public:
        /**
         * @brief 构造函数（constructor）；Create pipeline from config.
         * @param config Pipeline 配置（pipeline config）。
         */
        explicit MatrixBenchmarkPipeline(MatrixBenchmarkPipelineConfig config);

        /**
         * @brief 执行 pipeline（run pipeline）；Run full matrix-dot workflow and persist result CSV.
         * @return 写出的结果行（written result rows）。
         */
        [[nodiscard]] std::vector<domain::shared::BenchmarkResult> run() const override;

        /**
         * @brief 配置校验（validate config）；Check whether configuration satisfies minimal constraints.
         * @param config Pipeline 配置（pipeline config）。
         * @return 合法返回 true（true if valid）。
         */
        [[nodiscard]] static bool validate_config(const MatrixBenchmarkPipelineConfig &config) noexcept;

    private:
        /** @brief Pipeline 配置（pipeline config）；Owned runtime configuration. */
        MatrixBenchmarkPipelineConfig config_{};
    };

} // namespace cpu_lab::application
