#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace cpu_lab::domain::sum_reduce
{

    /**
     * @brief 超标量求和归约（superscalar sum reduction）；Reduce with multiple independent accumulation chains.
     * @param values 输入数组（input values）。
     * @param lane_count 链路数量（accumulation lane count），必须大于 0。
     * @return 归约和（reduced sum）。
     * @note lane_count 为 0 时抛出 std::invalid_argument（throws std::invalid_argument on invalid lane count）。
     */
    [[nodiscard]] double sum_superscalar(
        const std::vector<double> &values,
        std::size_t lane_count = 4U);

    /**
     * @brief sum_reduce 超标量策略（superscalar policy）；Policy wrapper using multi-lane accumulation.
     */
    class SumSuperscalar final
    {
    public:
        /**
         * @brief 构造函数（constructor）；Initialize owned input, lane config, and output cache.
         * @param values 输入数组（input values），按值持有。
         * @param lane_count 链路数量（accumulation lane count），必须大于 0。
         * @note lane_count 为 0 时抛出 std::invalid_argument（throws std::invalid_argument on invalid lane count）。
         */
        SumSuperscalar(std::vector<double> values, std::size_t lane_count = 4U);

        /**
         * @brief 获取算法名称（algorithm name）；Return stable policy name.
         * @return 算法名称（algorithm name）。
         */
        [[nodiscard]] std::string_view algorithm_name() const noexcept;

        /**
         * @brief 执行一次计算（run once）；Run one superscalar sum-reduce evaluation.
         * @return 无返回值；No return value.
         */
        void run_once();

        /**
         * @brief 获取最新输出（output）；Access latest reduced sum.
         * @return 最新输出（latest output）。
         */
        [[nodiscard]] double output() const noexcept;

        /**
         * @brief 输出指纹（output fingerprint）；Return 64-bit digest of latest output.
         * @return 64 位指纹（64-bit fingerprint）。
         */
        [[nodiscard]] std::uint64_t output_fingerprint() const noexcept;

    private:
        /** @brief 输入数组（input values）；Owned reduction input. */
        std::vector<double> values_{};

        /** @brief 链路数量（accumulation lane count）；Lane count used by superscalar kernel. */
        std::size_t lane_count_{4U};

        /** @brief 输出标量（output scalar）；Latest reduced sum. */
        double output_{0.0};
    };

} // namespace cpu_lab::domain::sum_reduce