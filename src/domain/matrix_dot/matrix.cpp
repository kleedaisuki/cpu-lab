#include "domain/matrix_dot/matrix.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cpu_lab::domain::matrix_dot
{

    namespace
    {
        /**
         * @brief 校验矩阵形状乘积（shape product guard）；Validate rows * cols does not overflow size_t.
         * @param rows 行数（row count）。
         * @param cols 列数（column count）。
         * @return 安全乘积（safe product）。
         * @note 溢出抛出 std::overflow_error（throws std::overflow_error on overflow）。
         */
        [[nodiscard]] std::size_t checked_product(const std::size_t rows, const std::size_t cols)
        {
            if (rows == 0U || cols == 0U)
            {
                return 0U;
            }

            if (rows > (std::numeric_limits<std::size_t>::max() / cols))
            {
                throw std::overflow_error("Matrix shape multiplication overflow.");
            }

            return rows * cols;
        }
    } // namespace

    Matrix::Matrix(const std::size_t rows, const std::size_t cols, const double value)
        : rows_(rows),
          cols_(cols),
          values_(checked_product(rows, cols), value)
    {
    }

    Matrix::Matrix(const std::size_t rows, const std::size_t cols, std::vector<double> values)
        : rows_(rows),
          cols_(cols),
          values_(std::move(values))
    {
        const std::size_t expected_size = checked_product(rows, cols);
        if (values_.size() != expected_size)
        {
            throw std::invalid_argument("Matrix storage size does not match shape.");
        }
    }

    std::size_t Matrix::rows() const noexcept
    {
        return rows_;
    }

    std::size_t Matrix::cols() const noexcept
    {
        return cols_;
    }

    std::size_t Matrix::size() const noexcept
    {
        return values_.size();
    }

    bool Matrix::empty() const noexcept
    {
        return values_.empty();
    }

    double &Matrix::at(const std::size_t row, const std::size_t col)
    {
        if (row >= rows_ || col >= cols_)
        {
            throw std::out_of_range("Matrix::at index out of range.");
        }
        return values_.at(linear_index(row, col));
    }

    const double &Matrix::at(const std::size_t row, const std::size_t col) const
    {
        if (row >= rows_ || col >= cols_)
        {
            throw std::out_of_range("Matrix::at index out of range.");
        }
        return values_.at(linear_index(row, col));
    }

    double &Matrix::operator()(const std::size_t row, const std::size_t col) noexcept
    {
        return values_[linear_index(row, col)];
    }

    const double &Matrix::operator()(const std::size_t row, const std::size_t col) const noexcept
    {
        return values_[linear_index(row, col)];
    }

    const std::vector<double> &Matrix::data() const noexcept
    {
        return values_;
    }

    std::vector<double> &Matrix::data() noexcept
    {
        return values_;
    }

    void Matrix::fill(const double value) noexcept
    {
        std::fill(values_.begin(), values_.end(), value);
    }

    std::size_t Matrix::linear_index(const std::size_t row, const std::size_t col) const noexcept
    {
        return (row * cols_) + col;
    }

} // namespace cpu_lab::domain::matrix_dot
