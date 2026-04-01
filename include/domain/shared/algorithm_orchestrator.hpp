#pragma once

#include "domain/shared/benchmark_result.hpp"
#include "infrastructure/timing/high_resolution_timer.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpu_lab::domain::shared
{

    /**
     * @brief 算法策略概念（algorithm policy concept）；Compile-time contract for pluggable algorithm policies.
     * @tparam PolicyT 策略类型（policy type）。
     */
    template <typename PolicyT>
    concept AlgorithmPolicy = requires(PolicyT &policy, const PolicyT &const_policy)
    {
        { const_policy.algorithm_name() } -> std::convertible_to<std::string_view>;
        { policy.run_once() } -> std::same_as<void>;
        { const_policy.output_fingerprint() } -> std::convertible_to<std::uint64_t>;
    };

    /**
     * @brief 编排器配置（orchestrator config）；Inputs for one benchmark orchestration.
     */
    struct AlgorithmOrchestratorConfig final
    {
        /** @brief 基准名称（benchmark name）；Scenario name used in output row. */
        std::string benchmark_name{};

        /** @brief 问题规模（problem size）；Input size label for this benchmark row. */
        std::size_t problem_size{0U};

        /** @brief 采样 run-count 列表（run-count list）；Run counts used by timer samples. */
        std::vector<std::size_t> run_counts{};

        /** @brief 预热轮数（warmup rounds）；Warmup rounds before timed measurement. */
        std::size_t warmup_rounds{1U};

        /** @brief 备注（notes）；Free-form notes merged into output row. */
        std::string notes{};
    };

    /**
     * @brief 算法编排器（algorithm orchestrator）；Domain service for timing algorithm policies.
     */
    class AlgorithmOrchestrator final
    {
    public:
        /**
         * @brief 执行一次基准编排（orchestrate one benchmark run），并由编排器管理策略生命周期。
         *        Orchestrate one benchmark and manage policy lifecycle inside orchestrator.
         * @tparam PolicyT 策略类型（policy type）。
         * @tparam CtorArgsT 策略构造参数类型（policy constructor argument types）。
         * @param config 编排配置（orchestrator config）。
         * @param ctor_args 策略构造参数（policy constructor arguments）。
         * @return 基准结果行（benchmark result row）。
         * @note 若配置不合法会抛出 std::invalid_argument（throws std::invalid_argument on invalid config）。
         */
        template <AlgorithmPolicy PolicyT, typename... CtorArgsT>
            requires std::constructible_from<PolicyT, CtorArgsT...>
        [[nodiscard]] BenchmarkResult operator()(
            const AlgorithmOrchestratorConfig &config,
            CtorArgsT &&...ctor_args) const
        {
            PolicyT policy(std::forward<CtorArgsT>(ctor_args)...);
            return measure_policy(policy, config);
        }

        /**
         * @brief 校验配置合法性（validate config）；Check whether config satisfies minimal constraints.
         * @param config 编排配置（orchestrator config）。
         * @return 合法返回 true（true if valid）。
         */
        [[nodiscard]] static bool validate_config(
            const AlgorithmOrchestratorConfig &config) noexcept;

    private:
        /**
         * @brief 对策略执行测量（measure policy）；Run timing and aggregate statistics for one policy.
         * @tparam PolicyT 策略类型（policy type）。
         * @param policy 策略实例（policy instance）。
         * @param config 编排配置（orchestrator config）。
         * @return 基准结果行（benchmark result row）。
         */
        template <AlgorithmPolicy PolicyT>
        [[nodiscard]] static BenchmarkResult measure_policy(
            PolicyT &policy,
            const AlgorithmOrchestratorConfig &config)
        {
            if (!validate_config(config)) {
                throw std::invalid_argument("AlgorithmOrchestratorConfig is invalid.");
            }

            infrastructure::timing::HighResolutionTimer timer{config.warmup_rounds};

            const auto report = timer.measure(
                [&policy]()
                {
                    policy.run_once();
                    std::atomic_signal_fence(std::memory_order_seq_cst);
                },
                config.run_counts);

            const auto per_run_ns = to_ns_per_run(report.samples);
            const double mean_ns = compute_mean(per_run_ns);

            BenchmarkResult result{};
            result.benchmark_name = config.benchmark_name;
            result.algorithm = std::string{policy.algorithm_name()};
            result.problem_size = config.problem_size;
            result.runs_per_sample = config.run_counts.front();
            result.sample_count = report.samples.size();
            result.best_ns_per_run = per_run_ns.empty()
                ? 0.0
                : *std::min_element(per_run_ns.begin(), per_run_ns.end());
            result.mean_ns_per_run = mean_ns;
            result.median_ns_per_run = compute_median(per_run_ns);
            result.stddev_ns = compute_stddev(per_run_ns, mean_ns);
            result.fit_slope_ns_per_run = report.fit.slope_ns_per_run;
            result.fit_intercept_ns = report.fit.intercept_ns;
            result.fit_r_squared = report.fit.r_squared;
            result.notes = compose_notes(config.notes, policy.output_fingerprint());
            return result;
        }

        /**
         * @brief 转换为单次运行耗时（ns/run）；Convert raw samples to per-run latency.
         * @param samples 原始采样点（raw timing samples）。
         * @return ns/run 序列（ns per run sequence）。
         */
        [[nodiscard]] static std::vector<double> to_ns_per_run(
            const std::vector<infrastructure::timing::TimingSample> &samples);

        /**
         * @brief 计算平均值（mean）；Compute arithmetic mean.
         * @param values 数值序列（value sequence）。
         * @return 平均值（mean value）。
         */
        [[nodiscard]] static double compute_mean(
            const std::vector<double> &values);

        /**
         * @brief 计算中位数（median）；Compute median.
         * @param values 数值序列（value sequence）。
         * @return 中位数（median value）。
         */
        [[nodiscard]] static double compute_median(
            std::vector<double> values);

        /**
         * @brief 计算标准差（standard deviation）；Compute population standard deviation.
         * @param values 数值序列（value sequence）。
         * @param mean 均值（mean value）。
         * @return 标准差（standard deviation）。
         */
        [[nodiscard]] static double compute_stddev(
            const std::vector<double> &values,
            double mean);

        /**
         * @brief 组合备注与指纹（compose notes with digest）；Append digest marker to notes.
         * @param notes 原始备注（original notes）。
         * @param digest 输出指纹（output digest）。
         * @return 组合后备注（composed notes）。
         */
        [[nodiscard]] static std::string compose_notes(
            const std::string &notes,
            std::uint64_t digest);
    };

} // namespace cpu_lab::domain::shared