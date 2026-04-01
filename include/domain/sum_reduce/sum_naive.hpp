#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace cpu_lab::domain::sum_reduce
{

    /**
     * @brief 朴素求和归约（naive sum reduction）；Accumulate all elements with one scalar chain.
     * @param values 输入数组（input values）。
     * @return 归约和（reduced sum）。
     */
    [[nodiscard]] double sum_naive(const std::vector<double> &values) noexcept;

    /**
     * @brief sum_reduce 朴素策略（naive policy）；Policy wrapper compatible with orchestrator concept.
     */
    class SumNaive final
    {
    public:
        /**
         * @brief 构造函数（constructor）；Initialize owned input and output cache.
         * @param values 输入数组（input values），按值持有。
         */
        explicit SumNaive(std::vector<double> values);

        /**
         * @brief 获取算法名称（algorithm name）；Return stable policy name.
         * @return 算法名称（algorithm name）。
         */
        [[nodiscard]] std::string_view algorithm_name() const noexcept;

        /**
         * @brief 执行一次计算（run once）；Run one sum-reduce evaluation.
         * @return 无返回值；No return value.
         */
        void run_once() noexcept;

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

        /** @brief 输出标量（output scalar）；Latest reduced sum. */
        double output_{0.0};
    };

} // namespace cpu_lab::domain::sum_reduce