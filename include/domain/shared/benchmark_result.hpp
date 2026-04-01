#pragma once

#include "infrastructure/csv/row.hpp"

#include <cstddef>
#include <string>
#include <tuple>

namespace cpu_lab::domain::shared
{

    /**
     * @brief 基准结果行（benchmark result row）；Shared benchmark output row with reflected CSV schema.
     */
    struct BenchmarkResult final : infrastructure::csv::Row<BenchmarkResult>
    {
        /** @brief 基准名称（benchmark name）；Benchmark scenario name. */
        std::string benchmark_name{};

        /** @brief 算法名称（algorithm name）；Algorithm or policy name. */
        std::string algorithm{};

        /** @brief 问题规模（problem size）；Input size for one run. */
        std::size_t problem_size{0U};

        /** @brief 每样本运行次数（runs per sample）；Runs aggregated in one timing sample. */
        std::size_t runs_per_sample{0U};

        /** @brief 样本数量（sample count）；Number of measured samples. */
        std::size_t sample_count{0U};

        /** @brief 最优单次耗时纳秒（best ns per run）；Best latency per run in nanoseconds. */
        double best_ns_per_run{0.0};

        /** @brief 平均单次耗时纳秒（mean ns per run）；Mean latency per run in nanoseconds. */
        double mean_ns_per_run{0.0};

        /** @brief 中位单次耗时纳秒（median ns per run）；Median latency per run in nanoseconds. */
        double median_ns_per_run{0.0};

        /** @brief 标准差纳秒（stddev ns）；Standard deviation in nanoseconds. */
        double stddev_ns{0.0};

        /** @brief 最小二乘斜率（least-squares slope）；Slope from least-squares fit (ns per run). */
        double fit_slope_ns_per_run{0.0};

        /** @brief 最小二乘截距（least-squares intercept）；Intercept from least-squares fit (ns). */
        double fit_intercept_ns{0.0};

        /** @brief 拟合优度（coefficient of determination）；R-squared quality metric. */
        double fit_r_squared{0.0};

        /** @brief 备注（notes）；Free-form notes for one benchmark row. */
        std::string notes{};

        /**
         * @brief 字段元信息（metadata）；Reflected field schema for CSV row mapping.
         * @return 字段描述元组（field descriptor tuple）。
         */
        [[nodiscard]] static auto meta() noexcept
            -> std::tuple<
                infrastructure::csv::RowField<BenchmarkResult, std::string>,
                infrastructure::csv::RowField<BenchmarkResult, std::string>,
                infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
                infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
                infrastructure::csv::RowField<BenchmarkResult, std::size_t>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, double>,
                infrastructure::csv::RowField<BenchmarkResult, std::string>>;

        /**
         * @brief 校验结果是否满足基本约束（validate row invariants）；Check basic numeric and semantic constraints.
         * @return 若满足约束返回 true；True if row invariants hold.
         */
        [[nodiscard]] bool is_valid() const noexcept;
    };

} // namespace cpu_lab::domain::shared