#include "domain/sum_reduce/sum_naive.hpp"

#include <bit>
#include <cstdint>
#include <utility>

namespace cpu_lab::domain::sum_reduce
{

    namespace
    {
        /**
         * @brief 混合 64 位哈希（64-bit hash mix）；Mix one 64-bit input into rolling hash.
         * @param seed 滚动种子（rolling seed）。
         * @param value 输入值（input value）。
         * @return 新种子（new seed）。
         */
        [[nodiscard]] std::uint64_t mix_hash(std::uint64_t seed, const std::uint64_t value) noexcept
        {
            seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6U) + (seed >> 2U);
            return seed;
        }
    } // namespace

    double sum_naive(const std::vector<double> &values) noexcept
    {
        double sum = 0.0;
        for (const double value : values)
        {
            sum += value;
        }
        return sum;
    }

    SumNaive::SumNaive(std::vector<double> values)
        : values_(std::move(values))
    {
    }

    std::string_view SumNaive::algorithm_name() const noexcept
    {
        return "sum_naive";
    }

    void SumNaive::run_once() noexcept
    {
        output_ = sum_naive(values_);
    }

    double SumNaive::output() const noexcept
    {
        return output_;
    }

    std::uint64_t SumNaive::output_fingerprint() const noexcept
    {
        std::uint64_t seed = 0x8E67D1B2D9A4F153ULL;
        seed = mix_hash(seed, std::bit_cast<std::uint64_t>(output_));
        seed = mix_hash(seed, static_cast<std::uint64_t>(values_.size()));
        return seed;
    }

} // namespace cpu_lab::domain::sum_reduce