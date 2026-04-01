#pragma once

#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief CUDA 矩阵列点积（CUDA matrix-column dot）；Compute dot(vector, each matrix column) on GPU.
     * @param matrix 输入矩阵（input matrix），形状为 M x N。
     * @param vector 输入向量（input vector），长度必须为 M。
     * @param threads_per_block 每个线程块线程数（threads per block），必须大于 0 且不超过设备上限。
     * @return 输出向量（output vector），长度为 N。
     * @note 尺寸不匹配或 CUDA 运行失败时抛出异常（throws on shape mismatch or CUDA runtime failure）。
     */
    [[nodiscard]] Vector matrix_dot_cuda(
        const Matrix &matrix,
        const Vector &vector,
        std::size_t threads_per_block = 256U);

    /**
     * @brief matrix_dot CUDA 策略（CUDA policy）；Policy wrapper backed by CUDA implementation.
     */
    class MatrixDotCuda final
    {
    public:
        /**
         * @brief 构造函数（constructor）；Initialize CUDA policy with owned inputs and output buffer.
         * @param matrix 输入矩阵（input matrix），按值持有。
         * @param vector 输入向量（input vector），按值持有。
         * @param threads_per_block 每个线程块线程数（threads per block），必须大于 0。
         * @note 尺寸不匹配时抛出 std::invalid_argument（throws std::invalid_argument on shape mismatch）。
         */
        MatrixDotCuda(Matrix matrix, Vector vector, std::size_t threads_per_block = 256U);

        /**
         * @brief 获取算法名称（algorithm name）；Return stable policy name.
         * @return 算法名称（algorithm name）。
         */
        [[nodiscard]] std::string_view algorithm_name() const noexcept;

        /**
         * @brief 执行一次计算（run once）；Run one CUDA matrix-dot evaluation.
         * @return 无返回值；No return value.
         */
        void run_once();

        /**
         * @brief 获取最新输出（output）；Access latest computed output vector.
         * @return 输出向量（output vector）。
         */
        [[nodiscard]] const Vector &output() const noexcept;

        /**
         * @brief 输出指纹（output fingerprint）；Return 64-bit digest of latest output.
         * @return 64 位指纹（64-bit fingerprint）。
         */
        [[nodiscard]] std::uint64_t output_fingerprint() const noexcept;

    private:
        /** @brief 输入矩阵（input matrix）；Owned matrix input. */
        Matrix matrix_{};

        /** @brief 输入向量（input vector）；Owned vector input. */
        Vector vector_{};

        /** @brief CUDA 线程块大小（CUDA block size）；Threads launched per block. */
        std::size_t threads_per_block_{256U};

        /** @brief 输出向量（output vector）；Reusable output buffer. */
        Vector output_{};
    };

} // namespace cpu_lab::domain::matrix_dot