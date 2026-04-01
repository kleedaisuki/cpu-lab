#include "domain/matrix_dot/matrix_dot_naive.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace cpu_lab::domain::matrix_dot
{

    Vector matrix_dot_naive(const Matrix &matrix, const Vector &vector)
    {
        if (matrix.rows() != vector.size()) {
            throw std::invalid_argument("matrix_dot_naive requires matrix.rows() == vector.size().");
        }

        Vector output(matrix.cols(), 0.0);
        for (std::size_t col = 0U; col < matrix.cols(); ++col) {
            double sum = 0.0;
            for (std::size_t row = 0U; row < matrix.rows(); ++row) {
                sum += matrix(row, col) * vector[row];
            }
            output[col] = sum;
        }

        return output;
    }

    MatrixDotNaive::MatrixDotNaive(Matrix matrix, Vector vector)
        : matrix_(std::move(matrix)),
          vector_(std::move(vector)),
          output_(matrix_.cols(), 0.0)
    {
        if (matrix_.rows() != vector_.size()) {
            throw std::invalid_argument("MatrixDotNaive requires matrix.rows() == vector.size().");
        }
    }

    std::string_view MatrixDotNaive::algorithm_name() const noexcept
    {
        return "matrix_dot_naive";
    }

    void MatrixDotNaive::run_once()
    {
        output_ = matrix_dot_naive(matrix_, vector_);
    }

    const Vector &MatrixDotNaive::output() const noexcept
    {
        return output_;
    }

    std::uint64_t MatrixDotNaive::output_fingerprint() const noexcept
    {
        return output_.fingerprint();
    }

} // namespace cpu_lab::domain::matrix_dot
