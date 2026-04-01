#pragma once

#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/vector.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief cache 友好矩阵列点积（cache-friendly matrix-column dot）；Compute dot(vector, each matrix column) with row-wise accumulation.
     * @param matrix 输入矩阵（input matrix），形状为 M x N。
     * @param vector 输入向量（input vector），长度必须为 M。
     * @param block_cols 列分块大小（column tile size），必须大于 0。
     * @return 输出向量（output vector），长度为 N。
     * @note 尺寸不匹配或 block_cols 为 0 时抛出 std::invalid_argument（throws std::invalid_argument on invalid input）。
     */
    [[nodiscard]] Vector matrix_dot_cache(
        const Matrix &matrix,
        const Vector &vector,
        std::size_t block_cols = 64U);

    /**
     * @brief matrix_dot cache 策略（cache policy）；Policy wrapper using cache-friendly loop order.
     */
    class MatrixDotCache final
    {
    public:
        /**
         * @brief 构造函数（constructor）；Initialize cache policy with owned inputs and output buffer.
         * @param matrix 输入矩阵（input matrix），按值持有。
         * @param vector 输入向量（input vector），按值持有。
         * @param block_cols 列分块大小（column tile size），必须大于 0。
         * @note 尺寸不匹配或 block_cols 为 0 时抛出 std::invalid_argument（throws std::invalid_argument on invalid input）。
         */
        MatrixDotCache(Matrix matrix, Vector vector, std::size_t block_cols = 64U);

        /**
         * @brief 获取算法名称（algorithm name）；Return stable policy name.
         * @return 算法名称（algorithm name）。
         */
        [[nodiscard]] std::string_view algorithm_name() const noexcept;

        /**
         * @brief 执行一次计算（run once）；Run one cache-friendly matrix-dot evaluation.
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

        /** @brief 列分块大小（column tile size）；Column tile width for cache-friendly traversal. */
        std::size_t block_cols_{64U};

        /** @brief 输出向量（output vector）；Reusable output buffer. */
        Vector output_{};
    };

} // namespace cpu_lab::domain::matrix_dot
