#include "domain/sum_reduce/sum_superscalar.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cpu_lab::domain::sum_reduce
{

    namespace
    {
        /**
         * @brief 校验链路参数（validate lane count）；Validate superscalar lane count argument.
         * @param lane_count 链路数量（accumulation lane count）。
         * @return 无返回值；No return value.
         */
        void validate_lane_count(const std::size_t lane_count)
        {
            if (lane_count == 0U)
            {
                throw std::invalid_argument("sum_superscalar requires lane_count > 0.");
            }
        }

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

        /**
         * @brief 超标量内核（superscalar kernel）；Reduce values into lane-local accumulators then fold.
         * @param values 输入数组（input values）。
         * @param lane_count 链路数量（accumulation lane count）。
         * @return 归约和（reduced sum）。
         */
        [[nodiscard]] double sum_superscalar_impl(
            const std::vector<double> &values,
            const std::size_t lane_count)
        {
            std::vector<double> lanes(lane_count, 0.0);

            std::size_t index = 0U;
            const std::size_t aligned_end = (values.size() / lane_count) * lane_count;
            for (; index < aligned_end; index += lane_count)
            {
                for (std::size_t lane = 0U; lane < lane_count; ++lane)
                {
                    lanes[lane] += values[index + lane];
                }
            }

            double sum = 0.0;
            for (const double lane_sum : lanes)
            {
                sum += lane_sum;
            }

            for (; index < values.size(); ++index)
            {
                sum += values[index];
            }

            return sum;
        }
    } // namespace

    double sum_superscalar(const std::vector<double> &values, const std::size_t lane_count)
    {
        validate_lane_count(lane_count);
        return sum_superscalar_impl(values, lane_count);
    }

    SumSuperscalar::SumSuperscalar(std::vector<double> values, const std::size_t lane_count)
        : values_(std::move(values)),
          lane_count_(lane_count)
    {
        validate_lane_count(lane_count_);
    }

    std::string_view SumSuperscalar::algorithm_name() const noexcept
    {
        return "sum_superscalar";
    }

    void SumSuperscalar::run_once()
    {
        output_ = sum_superscalar_impl(values_, lane_count_);
    }

    double SumSuperscalar::output() const noexcept
    {
        return output_;
    }

    std::uint64_t SumSuperscalar::output_fingerprint() const noexcept
    {
        std::uint64_t seed = 0x4F3A6CB12D98E075ULL;
        seed = mix_hash(seed, std::bit_cast<std::uint64_t>(output_));
        seed = mix_hash(seed, static_cast<std::uint64_t>(values_.size()));
        seed = mix_hash(seed, static_cast<std::uint64_t>(lane_count_));
        return seed;
    }

} // namespace cpu_lab::domain::sum_reduce