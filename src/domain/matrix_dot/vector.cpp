#include "domain/matrix_dot/vector.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace cpu_lab::domain::matrix_dot
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

    Vector::Vector(const std::size_t size, const double value)
        : values_(size, value)
    {
    }

    Vector::Vector(std::vector<double> values)
        : values_(std::move(values))
    {
    }

    std::size_t Vector::size() const noexcept
    {
        return values_.size();
    }

    bool Vector::empty() const noexcept
    {
        return values_.empty();
    }

    const std::vector<double> &Vector::data() const noexcept
    {
        return values_;
    }

    std::vector<double> &Vector::data() noexcept
    {
        return values_;
    }

    double &Vector::at(const std::size_t index)
    {
        return values_.at(index);
    }

    const double &Vector::at(const std::size_t index) const
    {
        return values_.at(index);
    }

    double &Vector::operator[](const std::size_t index) noexcept
    {
        return values_[index];
    }

    const double &Vector::operator[](const std::size_t index) const noexcept
    {
        return values_[index];
    }

    void Vector::fill(const double value) noexcept
    {
        std::fill(values_.begin(), values_.end(), value);
    }

    double Vector::dot(const Vector &other) const
    {
        if (values_.size() != other.values_.size())
        {
            throw std::invalid_argument("Vector::dot requires equal sizes.");
        }

        double sum = 0.0;
        for (std::size_t i = 0U; i < values_.size(); ++i)
        {
            sum += values_[i] * other.values_[i];
        }

        return sum;
    }

    std::uint64_t Vector::fingerprint() const noexcept
    {
        std::uint64_t seed = 0xC70F6907UL;
        for (const double value : values_)
        {
            seed = mix_hash(seed, std::bit_cast<std::uint64_t>(value));
        }
        return seed;
    }

} // namespace cpu_lab::domain::matrix_dot
