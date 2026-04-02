#include "domain/sum_reduce/sum_superscalar.hpp"

#include <algorithm>
#include <array>
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
         * @brief 编译期 lane 块累加（compile-time lane block accumulation）；Accumulate one aligned block into fixed lanes.
         * @tparam LaneCount 链路数量（lane count）。
         * @tparam LaneIndex 链路下标包（lane index pack）。
         * @param lanes 链路累加器（lane accumulators）。
         * @param values 输入数组（input values）。
         * @param index 当前块起始下标（current block base index）。
         * @return 无返回值；No return value.
         */
        template <std::size_t LaneCount, std::size_t... LaneIndex>
        inline void accumulate_lane_block(
            std::array<double, LaneCount> &lanes,
            const std::vector<double> &values,
            const std::size_t index,
            std::index_sequence<LaneIndex...>) noexcept
        {
            ((lanes[LaneIndex] += values[index + LaneIndex]), ...);
        }

        /**
         * @brief 编译期固定 lane 内核（fixed-lane kernel）；Reduce with compile-time lane count for better ILP optimization.
         * @tparam LaneCount 链路数量（lane count）。
         * @param values 输入数组（input values）。
         * @return 归约和（reduced sum）。
         */
        template <std::size_t LaneCount>
        [[nodiscard]] double sum_superscalar_fixed(const std::vector<double> &values) noexcept
        {
            std::array<double, LaneCount> lanes{};

            std::size_t index = 0U;
            const std::size_t aligned_end = values.size() - (values.size() % LaneCount);
            for (; index < aligned_end; index += LaneCount)
            {
                accumulate_lane_block(
                    lanes,
                    values,
                    index,
                    std::make_index_sequence<LaneCount>{});
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

        /**
         * @brief 运行期 lane 通用内核（runtime-lane generic kernel）；Reduce using runtime lane count and caller-provided lane buffer.
         * @param values 输入数组（input values）。
         * @param lanes 链路缓冲区指针（lane buffer pointer）。
         * @param lane_count 链路数量（lane count）。
         * @return 归约和（reduced sum）。
         */
        [[nodiscard]] double sum_superscalar_runtime_kernel(
            const std::vector<double> &values,
            double *const lanes,
            const std::size_t lane_count) noexcept
        {
            std::fill_n(lanes, lane_count, 0.0);

            std::size_t index = 0U;
            const std::size_t aligned_end = values.size() - (values.size() % lane_count);
            for (; index < aligned_end; index += lane_count)
            {
                for (std::size_t lane = 0U; lane < lane_count; ++lane)
                {
                    lanes[lane] += values[index + lane];
                }
            }

            double sum = 0.0;
            for (std::size_t lane = 0U; lane < lane_count; ++lane)
            {
                sum += lanes[lane];
            }

            for (; index < values.size(); ++index)
            {
                sum += values[index];
            }

            return sum;
        }

        /**
         * @brief 运行期 lane 分派（runtime-lane dispatch）；Use stack lane buffer when possible, heap fallback otherwise.
         * @param values 输入数组（input values）。
         * @param lane_count 链路数量（lane count）。
         * @return 归约和（reduced sum）。
         */
        [[nodiscard]] double sum_superscalar_runtime(
            const std::vector<double> &values,
            const std::size_t lane_count)
        {
            constexpr std::size_t kStackLaneLimit = 64U;

            if (lane_count <= kStackLaneLimit)
            {
                std::array<double, kStackLaneLimit> stack_lanes{};
                return sum_superscalar_runtime_kernel(values, stack_lanes.data(), lane_count);
            }

            std::vector<double> heap_lanes(lane_count, 0.0);
            return sum_superscalar_runtime_kernel(values, heap_lanes.data(), lane_count);
        }

        /**
         * @brief 超标量主分派（superscalar top-level dispatch）；Dispatch to fixed-lane fast paths first, fallback to generic runtime path.
         * @param values 输入数组（input values）。
         * @param lane_count 链路数量（lane count）。
         * @return 归约和（reduced sum）。
         */
        [[nodiscard]] double sum_superscalar_impl(
            const std::vector<double> &values,
            const std::size_t lane_count)
        {
            switch (lane_count)
            {
            case 1U:
                return sum_superscalar_fixed<1U>(values);
            case 2U:
                return sum_superscalar_fixed<2U>(values);
            case 4U:
                return sum_superscalar_fixed<4U>(values);
            case 8U:
                return sum_superscalar_fixed<8U>(values);
            case 16U:
                return sum_superscalar_fixed<16U>(values);
            default:
                return sum_superscalar_runtime(values, lane_count);
            }
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
