#pragma once

#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/vector.hpp"

#include <cstdint>
#include <string_view>

namespace cpu_lab::domain::matrix_dot
{

    /**
     * @brief 朴素矩阵列点积（naive matrix-column dot）；Compute dot(vector, each matrix column).
     * @param matrix 输入矩阵（input matrix），形状为 M x N。
     * @param vector 输入向量（input vector），长度必须为 M。
     * @return 输出向量（output vector），长度为 N。
     * @note 尺寸不匹配抛出 std::invalid_argument（throws std::invalid_argument on shape mismatch）。
     */
    [[nodiscard]] Vector matrix_dot_naive(const Matrix &matrix, const Vector &vector);

    /**
     * @brief matrix_dot 朴素策略（naive policy）；Policy wrapper compatible with orchestrator concept.
     */
    class MatrixDotNaive final
    {
    public:
        /**
         * @brief 构造函数（constructor）；Initialize policy inputs and internal output buffer.
         * @param matrix 输入矩阵（input matrix），按值持有。
         * @param vector 输入向量（input vector），按值持有。
         * @note 尺寸不匹配抛出 std::invalid_argument（throws std::invalid_argument on shape mismatch）。
         */
        MatrixDotNaive(Matrix matrix, Vector vector);

        /**
         * @brief 获取算法名称（algorithm name）；Return stable policy name.
         * @return 算法名称（algorithm name）。
         */
        [[nodiscard]] std::string_view algorithm_name() const noexcept;

        /**
         * @brief 执行一次计算（run once）；Run one matrix-dot evaluation.
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

        /** @brief 输出向量（output vector）；Reusable output buffer. */
        Vector output_{};
    };

} // namespace cpu_lab::domain::matrix_dot
