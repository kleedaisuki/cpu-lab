#include "domain/matrix_dot/matrix.hpp"
#include "domain/matrix_dot/matrix_dot_cache.hpp"
#include "domain/matrix_dot/matrix_dot_naive.hpp"
#include "domain/matrix_dot/vector.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

using cpu_lab::domain::matrix_dot::Matrix;
using cpu_lab::domain::matrix_dot::matrix_dot_cache;
using cpu_lab::domain::matrix_dot::matrix_dot_naive;
using cpu_lab::domain::matrix_dot::MatrixDotCache;
using cpu_lab::domain::matrix_dot::Vector;

namespace
{
    /**
     * @brief 比较浮点是否近似相等（approx equal）；Check if two floating-point values are approximately equal.
     * @param lhs 左值（left value）。
     * @param rhs 右值（right value）。
     * @param eps 容差（tolerance）。
     * @return 近似相等返回 true（true if approximately equal）。
     */
    [[nodiscard]] bool approx_equal(const double lhs, const double rhs, const double eps = 1e-12)
    {
        return std::abs(lhs - rhs) <= eps;
    }
}

int main()
{
    Matrix matrix(
        4U,
        5U,
        std::vector<double>{
            1.0,
            2.0,
            3.0,
            4.0,
            5.0,
            6.0,
            7.0,
            8.0,
            9.0,
            10.0,
            11.0,
            12.0,
            13.0,
            14.0,
            15.0,
            16.0,
            17.0,
            18.0,
            19.0,
            20.0,
        });

    Vector vector(std::vector<double>{0.5, -1.0, 2.0, 1.5});

    const Vector expected = matrix_dot_naive(matrix, vector);
    const Vector cached_default = matrix_dot_cache(matrix, vector);
    const Vector cached_block1 = matrix_dot_cache(matrix, vector, 1U);
    const Vector cached_block3 = matrix_dot_cache(matrix, vector, 3U);

    assert(expected.size() == cached_default.size());
    assert(expected.size() == cached_block1.size());
    assert(expected.size() == cached_block3.size());

    for (std::size_t i = 0U; i < expected.size(); ++i)
    {
        assert(approx_equal(expected[i], cached_default[i]));
        assert(approx_equal(expected[i], cached_block1[i]));
        assert(approx_equal(expected[i], cached_block3[i]));
    }

    MatrixDotCache policy(matrix, vector, 2U);
    assert(policy.algorithm_name() == "matrix_dot_cache");
    policy.run_once();
    const Vector &policy_output = policy.output();

    for (std::size_t i = 0U; i < expected.size(); ++i)
    {
        assert(approx_equal(expected[i], policy_output[i]));
    }
    assert(policy.output_fingerprint() == policy_output.fingerprint());

    bool threw_zero_block = false;
    try
    {
        (void)matrix_dot_cache(matrix, vector, 0U);
    }
    catch (const std::invalid_argument &)
    {
        threw_zero_block = true;
    }
    assert(threw_zero_block);

    bool threw_bad_shape = false;
    try
    {
        const Vector bad_vector(3U, 1.0);
        (void)matrix_dot_cache(matrix, bad_vector, 4U);
    }
    catch (const std::invalid_argument &)
    {
        threw_bad_shape = true;
    }
    assert(threw_bad_shape);

    return 0;
}
