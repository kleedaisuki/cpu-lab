#include "domain/matrix_dot/matrix_dot_cuda.hpp"

#include <stdexcept>
#include <utility>

namespace cpu_lab::domain::matrix_dot
{

    namespace
    {
        /**
         * @brief 无 CUDA 构建报错（throw on non-CUDA build）；Fail fast when CUDA policy is used without CUDA support.
         * @return 无返回值；No return value.
         */
        [[noreturn]] void throw_cuda_unavailable()
        {
            throw std::runtime_error(
                "matrix_dot_cuda is unavailable in this build. Reconfigure CMake with CUDA toolkit installed.");
        }

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
                throw std::invalid_argument("matrix_dot_cuda requires matrix.rows() == vector.size().");
            }
        }

        /**
         * @brief 校验线程块大小（validate block size）；Validate threads-per-block argument.
         * @param threads_per_block 每个线程块线程数（threads per block）。
         * @return 无返回值；No return value.
         */
        void validate_block_size(const std::size_t threads_per_block)
        {
            if (threads_per_block == 0U)
            {
                throw std::invalid_argument("matrix_dot_cuda requires threads_per_block > 0.");
            }
        }
    } // namespace

    Vector matrix_dot_cuda(
        const Matrix &matrix,
        const Vector &vector,
        const std::size_t threads_per_block)
    {
        validate_shape(matrix, vector);
        validate_block_size(threads_per_block);
        throw_cuda_unavailable();
    }

    MatrixDotCuda::MatrixDotCuda(Matrix matrix, Vector vector, const std::size_t threads_per_block)
        : matrix_(std::move(matrix)),
          vector_(std::move(vector)),
          threads_per_block_(threads_per_block),
          output_(matrix_.cols(), 0.0)
    {
        validate_shape(matrix_, vector_);
        validate_block_size(threads_per_block_);
    }

    std::string_view MatrixDotCuda::algorithm_name() const noexcept
    {
        return "matrix_dot_cuda";
    }

    void MatrixDotCuda::run_once()
    {
        output_ = matrix_dot_cuda(matrix_, vector_, threads_per_block_);
    }

    const Vector &MatrixDotCuda::output() const noexcept
    {
        return output_;
    }

    std::uint64_t MatrixDotCuda::output_fingerprint() const noexcept
    {
        return output_.fingerprint();
    }

} // namespace cpu_lab::domain::matrix_dot