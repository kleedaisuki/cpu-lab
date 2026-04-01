#include "domain/matrix_dot/matrix_dot_cache.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace cpu_lab::domain::matrix_dot
{

    namespace
    {
        /**
         * @brief 校验输入形状（validate shape）；Validate matrix/vector compatibility.
         * @param matrix 输入矩阵（input matrix）。
         * @param vector 输入向量（input vector）。
         * @return 无返回值；No return value.
         */
        void validate_shape(const Matrix &matrix, const Vector &vector)
        {
            if (matrix.rows() != vector.size())
            {
                throw std::invalid_argument("matrix_dot_cache requires matrix.rows() == vector.size().");
            }
        }

        /**
         * @brief 校验分块大小（validate block size）；Validate tile width argument.
         * @param block_cols 列分块大小（column tile size）。
         * @return 无返回值；No return value.
         */
        void validate_block_size(const std::size_t block_cols)
        {
            if (block_cols == 0U)
            {
                throw std::invalid_argument("matrix_dot_cache requires block_cols > 0.");
            }
        }

        /**
         * @brief cache 友好内核（cache-friendly kernel）；Accumulate per-row contributions into output.
         * @param matrix 输入矩阵（input matrix）。
         * @param vector 输入向量（input vector）。
         * @param block_cols 列分块大小（column tile size）。
         * @param output 输出缓冲区（output buffer）。
         * @return 无返回值；No return value.
         */
        void matrix_dot_cache_into(
            const Matrix &matrix,
            const Vector &vector,
            const std::size_t block_cols,
            Vector &output)
        {
            output.fill(0.0);

            for (std::size_t row = 0U; row < matrix.rows(); ++row)
            {
                const double v = vector[row];

                for (std::size_t col_base = 0U; col_base < matrix.cols(); col_base += block_cols)
                {
                    const std::size_t col_end = std::min(col_base + block_cols, matrix.cols());
                    for (std::size_t col = col_base; col < col_end; ++col)
                    {
                        output[col] += matrix(row, col) * v;
                    }
                }
            }
        }
    } // namespace

    Vector matrix_dot_cache(
        const Matrix &matrix,
        const Vector &vector,
        const std::size_t block_cols)
    {
        validate_shape(matrix, vector);
        validate_block_size(block_cols);

        Vector output(matrix.cols(), 0.0);
        matrix_dot_cache_into(matrix, vector, block_cols, output);
        return output;
    }

    MatrixDotCache::MatrixDotCache(Matrix matrix, Vector vector, const std::size_t block_cols)
        : matrix_(std::move(matrix)),
          vector_(std::move(vector)),
          block_cols_(block_cols),
          output_(matrix_.cols(), 0.0)
    {
        validate_shape(matrix_, vector_);
        validate_block_size(block_cols_);
    }

    std::string_view MatrixDotCache::algorithm_name() const noexcept
    {
        return "matrix_dot_cache";
    }

    void MatrixDotCache::run_once()
    {
        matrix_dot_cache_into(matrix_, vector_, block_cols_, output_);
    }

    const Vector &MatrixDotCache::output() const noexcept
    {
        return output_;
    }

    std::uint64_t MatrixDotCache::output_fingerprint() const noexcept
    {
        return output_.fingerprint();
    }

} // namespace cpu_lab::domain::matrix_dot
