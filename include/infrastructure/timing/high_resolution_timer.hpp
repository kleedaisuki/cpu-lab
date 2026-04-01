#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace cpu_lab::infrastructure::timing
{

    /**
     * @brief 单次采样点（sample）信息；Single benchmark sample point.
     */
    struct TimingSample
    {
        /** @brief 重复执行次数（run count）；Number of repeated runs in this sample. */
        std::size_t run_count{0U};

        /** @brief 总耗时纳秒（elapsed nanoseconds）；Total elapsed time in nanoseconds. */
        double elapsed_ns{0.0};
    };

    /**
     * @brief 最小二乘拟合结果（least-squares fit result）；Least-squares fitting result.
     */
    struct LeastSquaresFit
    {
        /** @brief 斜率 ns/run（slope）；Estimated time per run in nanoseconds. */
        double slope_ns_per_run{0.0};

        /** @brief 截距 ns（intercept）；Estimated fixed overhead in nanoseconds. */
        double intercept_ns{0.0};

        /** @brief 决定系数 R²（coefficient of determination）；Goodness-of-fit metric. */
        double r_squared{0.0};
    };

    /**
     * @brief 计时输出（timer report）；High-resolution timer output report.
     */
    struct TimerReport
    {
        /** @brief 原始采样（raw samples）；Collected sample points. */
        std::vector<TimingSample> samples{};

        /** @brief 最小二乘拟合（least-squares fit）；Linear fit of elapsed time vs run count. */
        LeastSquaresFit fit{};
    };

    /**
     * @brief 高精度计时器（high-resolution timer），支持 warmup 与最小二乘拟合。
     *        High-resolution timer with warmup and least-squares fitting.
     */
    class HighResolutionTimer
    {
    public:
        /**
         * @brief 构造计时器；Construct a timer.
         * @param warmup_rounds 预热轮数（warmup rounds），避免冷启动噪声。
         *                     Number of warmup rounds to reduce cold-start noise.
         */
        explicit HighResolutionTimer(std::size_t warmup_rounds = 1U) noexcept;

        /**
         * @brief 对任务执行多组 run-count 测量，并输出拟合结果。
         *        Measure a task with multiple run counts and produce fit report.
         * @param task 被测任务（callable task），每次调用执行一次逻辑单元。
         *            Callable task where one invocation represents one logical run.
         * @param run_counts 每个采样点的重复次数（run counts per sample）。
         *                  Repetition counts for each sample point.
         * @return 计时报告（timer report），包含原始样本与拟合参数。
         *         Timer report containing raw samples and fitted parameters.
         * @note task 不应抛出异常（exception），否则计时语义可能被打断。
         *       The task should avoid throwing exceptions during measurement.
         */
        [[nodiscard]] TimerReport measure(
            const std::function<void()> &task,
            const std::vector<std::size_t> &run_counts) const;

        /**
         * @brief 生成默认 run-count 序列（几何级数风格）。
         *        Build a default geometric-like run-count sequence.
         * @param first 首项（first run count）。
         * @param ratio 乘子（ratio）>= 2 时更稳健。
         * @param points 采样点数量（number of points）。
         * @return 递增 run-count 序列；Monotonically increasing run-count vector.
         */
        [[nodiscard]] static std::vector<std::size_t> build_default_run_counts(
            std::size_t first,
            std::size_t ratio,
            std::size_t points);

        /**
         * @brief 对给定样本执行线性最小二乘拟合（linear least squares）。
         *        Fit a line y = ax + b for elapsed time over run count.
         * @param samples 采样点集合（samples）。
         * @return 拟合结果（fit result）。
         */
        [[nodiscard]] static LeastSquaresFit fit_linear_least_squares(
            const std::vector<TimingSample> &samples) noexcept;

    private:
        /** @brief 预热轮数（warmup rounds）；Number of warmup rounds before measurement. */
        std::size_t warmup_rounds_{1U};

        /**
         * @brief 执行指定次数任务并返回耗时（ns）。
         *        Run task for run_count times and return elapsed nanoseconds.
         * @param task 被测任务；Task to execute.
         * @param run_count 执行次数；Number of invocations.
         * @return 总耗时纳秒；Total elapsed nanoseconds.
         */
        [[nodiscard]] static double run_once(
            const std::function<void()> &task,
            std::size_t run_count);
    };

} // namespace cpu_lab::infrastructure::timing
